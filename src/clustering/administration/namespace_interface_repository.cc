// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/administration/namespace_interface_repository.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/timing.hpp"
#include "clustering/query_routing/table_query_client.hpp"
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
        watchable_map_t<directory_key_t, table_query_bcard_t> *_directory,
        multi_table_manager_t *_multi_table_manager,
        rdb_context_t *_ctx,
        table_meta_client_t *table_meta_client)
    : mailbox_manager(_mailbox_manager),
      directory(_directory),
      multi_table_manager(_multi_table_manager),
      ctx(_ctx),
      m_table_meta_client(table_meta_client)
      { }

namespace_repo_t::~namespace_repo_t() { }

void copy_region_maps_to_thread(
        const std::map<namespace_id_t, std::map<key_range_t, server_id_t> > &from,
        one_per_thread_t<std::map<namespace_id_t, std::map<key_range_t, server_id_t> > > *to,
        int thread, UNUSED auto_drainer_t::lock_t keepalive) {
    on_thread_t th((threadnum_t(thread)));
    *to->get() = from;
}

void namespace_repo_t::create_and_destroy_namespace_interface(
            namespace_cache_t *cache,
            const namespace_id_t &table_id,
            auto_drainer_t::lock_t keepalive)
            THROWS_NOTHING {
    keepalive.assert_is_holding(&cache->drainer);
    threadnum_t thread = get_thread_id();

    namespace_cache_entry_t *cache_entry =
        cache->entries.find(table_id)->second.get();
    guarantee(!cache_entry->namespace_interface.get_ready_signal()->is_pulsed());

    /* We want to extract the entries in the directory that are for this table. This is
    the job of `table_directory_converter_t`. Since `directory` is thread-local, we have
    to construct it on the home thread. So we switch to the home thread, construct it and
    a `cross_thread_watchable_map_var_t`, then switch back. When the destructors are
    called, we do the same in reverse, making good use of RAII. */
    on_thread_t switch_to_home_thread(home_thread());
    class table_directory_converter_t :
        public watchable_map_transform_t<
            directory_key_t, table_query_bcard_t,
            std::pair<peer_id_t, uuid_u>, table_query_bcard_t> {
    public:
        table_directory_converter_t(
                watchable_map_t<directory_key_t, table_query_bcard_t> *_directory,
                const namespace_id_t &_table_id) :
            watchable_map_transform_t(_directory),
            table_id(_table_id)
            { }
        bool key_1_to_2(
                const directory_key_t &key1,
                std::pair<peer_id_t, uuid_u> *key2_out) {
            if (key1.second.first == table_id) {
                key2_out->first = key1.first;
                key2_out->second = key1.second.second;
                return true;
            } else {
                return false;
            }
        }
        void value_1_to_2(
                const table_query_bcard_t *value1,
                const table_query_bcard_t **value2_out) {
            *value2_out = value1;
        }
        bool key_2_to_1(
                const std::pair<peer_id_t, uuid_u> &key2,
                directory_key_t *key1_out) {
            key1_out->first = key2.first;
            key1_out->second.first = table_id;
            key1_out->second.second = key2.second;
            return true;
        }
    private:
        namespace_id_t table_id;
    } directory_converter_on_home_thread(directory, table_id);
    cross_thread_watchable_map_var_t<std::pair<peer_id_t, uuid_u>, table_query_bcard_t>
        directory_converter_on_this_thread(&directory_converter_on_home_thread, thread);
    on_thread_t switch_back(thread);

    table_query_client_t query_client(
        table_id,
        mailbox_manager,
        directory_converter_on_this_thread.get_watchable(),
        multi_table_manager,
        ctx,
        m_table_meta_client);

    try {
        /* Wait for the table to become available for use */
        wait_interruptible(query_client.get_initial_ready_signal(),
            keepalive.get_drain_signal());

        /* Give the outside world access to `query_client` */
        cache_entry->namespace_interface.pulse(&query_client);

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
    cache->entries.erase(table_id);
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

