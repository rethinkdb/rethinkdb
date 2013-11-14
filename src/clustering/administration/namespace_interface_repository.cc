// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/administration/namespace_interface_repository.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "arch/timing.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "clustering/reactor/namespace_interface.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"

#define NAMESPACE_INTERFACE_EXPIRATION_MS (60 * 1000)

template <class protocol_t>
struct namespace_repo_t<protocol_t>::namespace_cache_t {
public:
    boost::ptr_map<namespace_id_t, typename base_namespace_repo_t<protocol_t>::namespace_cache_entry_t> entries;
    auto_drainer_t drainer;
};


template <class protocol_t>
namespace_repo_t<protocol_t>::namespace_repo_t(mailbox_manager_t *_mailbox_manager,
                                               clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > > _namespaces_directory_metadata,
                                               typename protocol_t::context_t *_ctx)
    : mailbox_manager(_mailbox_manager),
      namespaces_directory_metadata(_namespaces_directory_metadata),
      ctx(_ctx)
{ }

template <class protocol_t>
namespace_repo_t<protocol_t>::~namespace_repo_t() { }

template <class protocol_t>
std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > get_reactor_business_cards(
        const std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > &ns_directory_metadata, const namespace_id_t &n_id) {
    std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > res;
    for (typename std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> >::const_iterator it  = ns_directory_metadata.begin();
                                                                                                    it != ns_directory_metadata.end();
                                                                                                    it++) {
        typename namespaces_directory_metadata_t<protocol_t>::reactor_bcards_map_t::const_iterator jt =
            it->second.reactor_bcards.find(n_id);
        if (jt != it->second.reactor_bcards.end()) {
            res[it->first] = jt->second.internal;
        } else {
            res[it->first] = cow_ptr_t<reactor_business_card_t<protocol_t> >();
        }
    }

    return res;
}

template <class protocol_t>
base_namespace_repo_t<protocol_t>::access_t::access_t() :
    cache_entry(NULL),
    thread(INVALID_THREAD)
    { }

template<class protocol_t>
base_namespace_repo_t<protocol_t>::access_t::access_t(base_namespace_repo_t *parent, const uuid_u &namespace_id, signal_t *interruptor) :
    thread(get_thread_id())
{
    {
        ASSERT_FINITE_CORO_WAITING;
        cache_entry = parent->get_cache_entry(namespace_id);
        ref_handler.init(cache_entry);
    }
    wait_interruptible(cache_entry->namespace_if.get_ready_signal(), interruptor);
}

template <class protocol_t>
base_namespace_repo_t<protocol_t>::access_t::access_t(const access_t& access) :
    cache_entry(access.cache_entry),
    thread(access.thread)
{
    if (cache_entry) {
        rassert(get_thread_id() == thread);
        ref_handler.init(cache_entry);
    }
}

template <class protocol_t>
typename base_namespace_repo_t<protocol_t>::access_t &base_namespace_repo_t<protocol_t>::access_t::operator=(const access_t &access) {
    if (this != &access) {
        cache_entry = access.cache_entry;
        ref_handler.reset();
        if (access.cache_entry) {
            ref_handler.init(access.cache_entry);
        }
        thread = access.thread;
    }
    return *this;
}

template <class protocol_t>
namespace_interface_t<protocol_t> *base_namespace_repo_t<protocol_t>::access_t::get_namespace_if() {
    rassert(thread == get_thread_id());
    return cache_entry->namespace_if.wait();
}

template <class protocol_t>
base_namespace_repo_t<protocol_t>::access_t::ref_handler_t::ref_handler_t() :
    ref_target(NULL) { }

template <class protocol_t>
base_namespace_repo_t<protocol_t>::access_t::ref_handler_t::~ref_handler_t() {
    reset();
}

template <class protocol_t>
void base_namespace_repo_t<protocol_t>::access_t::ref_handler_t::init(namespace_cache_entry_t *_ref_target) {
    ASSERT_NO_CORO_WAITING;
    guarantee(ref_target == NULL);
    ref_target = _ref_target;
    ref_target->ref_count++;
    if (ref_target->ref_count == 1) {
        if (ref_target->pulse_when_ref_count_becomes_nonzero) {
            ref_target->pulse_when_ref_count_becomes_nonzero->pulse_if_not_already_pulsed();
        }
    }
}

template <class protocol_t>
void base_namespace_repo_t<protocol_t>::access_t::ref_handler_t::reset() {
    ASSERT_NO_CORO_WAITING;
    if (ref_target != NULL) {
        ref_target->ref_count--;
        if (ref_target->ref_count == 0) {
            if (ref_target->pulse_when_ref_count_becomes_zero) {
                ref_target->pulse_when_ref_count_becomes_zero->pulse_if_not_already_pulsed();
            }
        }
    }
}

template <class protocol_t>
void namespace_repo_t<protocol_t>::create_and_destroy_namespace_interface(
            namespace_cache_t *cache,
            const uuid_u &namespace_id,
            auto_drainer_t::lock_t keepalive)
            THROWS_NOTHING{
    keepalive.assert_is_holding(&cache->drainer);
    threadnum_t thread = get_thread_id();

    typename base_namespace_repo_t<protocol_t>::namespace_cache_entry_t *cache_entry = cache->entries.find(namespace_id)->second;
    guarantee(!cache_entry->namespace_if.get_ready_signal()->is_pulsed());

    /* We need to switch to `home_thread()` to construct
    `cross_thread_watchable`, then switch back. In destruction we need to do the
    reverse. Fortunately RAII works really nicely here. */
    on_thread_t switch_to_home_thread(home_thread());
    clone_ptr_t<watchable_t<std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > > > subview =
        namespaces_directory_metadata->subview(boost::bind(&get_reactor_business_cards<protocol_t>, _1, namespace_id));
    cross_thread_watchable_variable_t<std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > > cross_thread_watchable(subview, thread);
    on_thread_t switch_back(thread);

    cluster_namespace_interface_t<protocol_t> namespace_interface(
        mailbox_manager,
        cross_thread_watchable.get_watchable(),
        ctx);

    try {
        wait_interruptible(namespace_interface.get_initial_ready_signal(),
            keepalive.get_drain_signal());

        /* Notify `access_t`s that the namespace is available now */
        cache_entry->namespace_if.pulse(&namespace_interface);

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
}

template <class protocol_t>
typename base_namespace_repo_t<protocol_t>::namespace_cache_entry_t *namespace_repo_t<protocol_t>::get_cache_entry(const uuid_u &ns_id) {
    typename base_namespace_repo_t<protocol_t>::namespace_cache_entry_t *cache_entry;
    namespace_cache_t *cache = namespace_caches.get();
    if (cache->entries.find(ns_id) == cache->entries.end()) {
        cache_entry = new typename base_namespace_repo_t<protocol_t>::namespace_cache_entry_t;
        cache_entry->ref_count = 0;
        cache_entry->pulse_when_ref_count_becomes_zero = NULL;
        cache_entry->pulse_when_ref_count_becomes_nonzero = NULL;

        namespace_id_t id(ns_id);
        cache->entries.insert(id, cache_entry);

        coro_t::spawn_sometime(boost::bind(
            &namespace_repo_t<protocol_t>::create_and_destroy_namespace_interface, this,
            cache, ns_id,
            auto_drainer_t::lock_t(&cache->drainer)));
    } else {
        cache_entry = &cache->entries[ns_id];
    }

    return cache_entry;
}

#include "mock/dummy_protocol.hpp"
#include "memcached/protocol.hpp"
#include "rdb_protocol/protocol.hpp"

template class base_namespace_repo_t<mock::dummy_protocol_t>;
template class base_namespace_repo_t<memcached_protocol_t>;
template class base_namespace_repo_t<rdb_protocol_t>;
template class namespace_repo_t<mock::dummy_protocol_t>;
template class namespace_repo_t<memcached_protocol_t>;
template class namespace_repo_t<rdb_protocol_t>;
