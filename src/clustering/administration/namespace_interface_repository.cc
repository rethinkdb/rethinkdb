// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/administration/namespace_interface_repository.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/timing.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"

#define NAMESPACE_INTERFACE_EXPIRATION_MS (60 * 1000)

struct namespace_repo_t::namespace_cache_t {
public:
    std::map<namespace_id_t, scoped_ptr_t<namespace_cache_entry_t> > entries;
    auto_drainer_t drainer;
};

struct namespace_repo_t::namespace_cache_entry_t :
    public namespace_interface_access_t::ref_tracker_t
{
public:
    void add_ref() {
        ref_count++;
        if (ref_count == 1) {
            if (pulse_when_ref_count_becomes_nonzero) {
                pulse_when_ref_count_becomes_nonzero->
                    pulse_if_not_already_pulsed();
            }
        }
    }
    void release() {
        ref_count--;
        if (ref_count == 0) {
            if (pulse_when_ref_count_becomes_zero) {
                pulse_when_ref_count_becomes_zero->
                    pulse_if_not_already_pulsed();
            }
        }
    }

    promise_t<namespace_interface_t *> namespace_interface;
    int ref_count;
    cond_t *pulse_when_ref_count_becomes_zero;
    cond_t *pulse_when_ref_count_becomes_nonzero;
};

namespace_repo_t::namespace_repo_t(
        mailbox_manager_t *_mailbox_manager,
        table_meta_client_t *_table_meta_client,
        rdb_context_t *_ctx)
    : mailbox_manager(_mailbox_manager),
      table_meta_client(_table_meta_client),
      ctx(_ctx)
      // RSI(raft): Reimplement this once table IO works
      // namespaces_subscription(boost::bind(&namespace_repo_t::on_namespaces_change, this, drainer.lock()))
{
    // RSI(raft): Reimplement this once table IO works
    // namespaces_subscription.reset(namespaces_view);
}

namespace_repo_t::~namespace_repo_t() { }

void copy_region_maps_to_thread(
        const std::map<namespace_id_t, std::map<key_range_t, server_id_t> > &from,
        one_per_thread_t<std::map<namespace_id_t, std::map<key_range_t, server_id_t> > > *to,
        int thread, UNUSED auto_drainer_t::lock_t keepalive) {
    on_thread_t th((threadnum_t(thread)));
    *to->get() = from;
}

// RSI(raft): Reimplement once table IO works
#if 0
void namespace_repo_t::on_namespaces_change(auto_drainer_t::lock_t keepalive) {
    ASSERT_NO_CORO_WAITING;
    std::map<namespace_id_t, std::map<key_range_t, server_id_t> > new_reg_to_pri_maps;

    namespaces_semilattice_metadata_t::namespace_map_t::const_iterator it;
    const namespaces_semilattice_metadata_t::namespace_map_t &ns = namespaces_view.get()->get().get()->namespaces;
    for (it = ns.begin(); it != ns.end(); ++it) {
        if (it->second.is_deleted()) {
            continue;
        }
        table_replication_info_t info = it->second.get_ref().replication_info.get_ref();
        for (size_t i = 0; i < info.config.shards.size(); ++i) {
            new_reg_to_pri_maps[it->first][info.shard_scheme.get_shard_range(i)] =
                info.config.shards[i].primary_replica;
        }
    }

    for (int thread = 0; thread < get_num_threads(); ++thread) {
        coro_t::spawn_ordered(std::bind(&copy_region_maps_to_thread,
                                        new_reg_to_pri_maps,
                                        &region_to_primary_maps,
                                        thread,
                                        keepalive));
    }
}
#endif

void namespace_repo_t::create_and_destroy_namespace_interface(
            UNUSED namespace_cache_t *cache,
            UNUSED const uuid_u &namespace_id,
            UNUSED auto_drainer_t::lock_t keepalive)
            THROWS_NOTHING {
    /* RSI(raft): Re-implement this after Raft is ready to support table IO again. */
    not_implemented();
    (void)mailbox_manager;
    (void)table_meta_client;
    (void)ctx;
#if 0
    keepalive.assert_is_holding(&cache->drainer);
    threadnum_t thread = get_thread_id();

    namespace_cache_entry_t *cache_entry = cache->entries.find(namespace_id)->second.get();
    guarantee(!cache_entry->namespace_interface.get_ready_signal()->is_pulsed());

    /* We need to switch to `home_thread()` to construct `cross_thread_watchable`, then
    switch back. In destruction we need to do the reverse. Fortunately RAII works really
    nicely here. */
    on_thread_t switch_to_home_thread(home_thread());

    table_directory_converter_t table_directory(directory, namespace_id);
    cross_thread_watchable_map_var_t<peer_id_t, namespace_directory_metadata_t>
        cross_thread_watchable(&table_directory, thread);
    on_thread_t switch_back(thread);

    cluster_namespace_interface_t namespace_interface(
        mailbox_manager,
        region_to_primary_maps.get(),
        cross_thread_watchable.get_watchable(),
        namespace_id,
        ctx);

    try {
        /* Wait for the table to become available for use */
        wait_interruptible(namespace_interface.get_initial_ready_signal(),
            keepalive.get_drain_signal());

        /* Give the outside world access to `namespace_interface` */
        cache_entry->namespace_interface.pulse(&namespace_interface);

        /* Wait until it's time to shut down */
        while (true) {
            while (cache_entry->ref_count != 0) {
                cond_t ref_count_is_zero;
                assignment_sentry_t<cond_t *> notify_if_ref_count_becomes_zero(
                    &cache_entry->pulse_when_ref_count_becomes_zero,
                    &ref_count_is_zero);
                wait_interruptible(&ref_count_is_zero, keepalive.get_drain_signal());
            }
            signal_timer_t expiration_timer;
            expiration_timer.start(NAMESPACE_INTERFACE_EXPIRATION_MS);
            cond_t ref_count_is_nonzero;
            assignment_sentry_t<cond_t *> notify_if_ref_count_becomes_nonzero(
                &cache_entry->pulse_when_ref_count_becomes_nonzero,
                &ref_count_is_nonzero);
            wait_any_t waiter(&expiration_timer, &ref_count_is_nonzero);
            wait_interruptible(&waiter, keepalive.get_drain_signal());
            if (!ref_count_is_nonzero.is_pulsed()) {
                guarantee(cache_entry->ref_count == 0);
                /* We waited a whole `NAMESPACE_INTERFACE_EXPIRATION_MS` and
                nothing used us. So let's destroy ourselves. */
                break;
            }
        }

    } catch (const interrupted_exc_t &) {
        /* We got here because we were interrupted in the startup process. That
        means the `namespace_repo_t` destructor was called, which means there
        mustn't exist any `access_t` objects. So ref_count must be 0. */
        guarantee(cache_entry->ref_count == 0);
    }

    ASSERT_NO_CORO_WAITING;
    cache->entries.erase(namespace_id);
#endif
}

namespace_interface_access_t namespace_repo_t::get_namespace_interface(
        const uuid_u &ns_id, signal_t *interruptor) {
    /* Find or create a cache entry for the table. When we find or create the cache, we
    need to wait until the `namespace_interface_t *` is actually ready before returning,
    but we want to be sure to hold a reference to the cache entry in the meantime. So we
    construct `temporary_holder`, which manages a reference to the `cache_entry`, but has
    its namespace interface set to `NULL`. Then when the real table is ready, we
    construct a real `namespace_interface_access_t` with a non-`NULL` namespace
    interface, and then delete `temporary_holder`. */
    namespace_interface_access_t temporary_holder;
    namespace_cache_entry_t *cache_entry;
    {
        ASSERT_FINITE_CORO_WAITING;
        namespace_cache_t *cache = namespace_caches.get();
        if (cache->entries.find(ns_id) == cache->entries.end()) {
            cache_entry = new namespace_cache_entry_t;
            cache_entry->ref_count = 0;
            cache_entry->pulse_when_ref_count_becomes_zero = NULL;
            cache_entry->pulse_when_ref_count_becomes_nonzero = NULL;

            namespace_id_t id(ns_id);
            cache->entries.insert(std::make_pair(id,
                scoped_ptr_t<namespace_cache_entry_t>(cache_entry)));

            coro_t::spawn_sometime(boost::bind(
                &namespace_repo_t::create_and_destroy_namespace_interface, this,
                cache, ns_id,
                auto_drainer_t::lock_t(&cache->drainer)));
        } else {
            cache_entry = cache->entries[ns_id].get();
        }
    }
    wait_interruptible(cache_entry->namespace_interface.get_ready_signal(), interruptor);
    return namespace_interface_access_t(
        cache_entry->namespace_interface.wait(),
        cache_entry,
        get_thread_id());
}

