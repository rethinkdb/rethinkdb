// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/reactor/directory_echo.hpp"

#include <functional>
#include <map>

template<class internal_t>
directory_echo_writer_t<internal_t>::our_value_change_t::our_value_change_t(directory_echo_writer_t *p) :
        parent(p), lock_acq(&parent->value_lock), buffer(parent->value_watchable.get_watchable()->get().internal) { }

template <class internal_t>
directory_echo_version_t directory_echo_writer_t<internal_t>::our_value_change_t::commit() {
    parent->version++;
    parent->value_watchable.set_value_no_equals(
        directory_echo_wrapper_t<internal_t>(
            buffer, parent->version, parent->ack_mailbox.get_address()));
    return parent->version;
}

template<class internal_t>
directory_echo_writer_t<internal_t>::ack_waiter_t::ack_waiter_t(directory_echo_writer_t *parent, peer_id_t peer, directory_echo_version_t _version) {
    DEBUG_VAR mutex_assertion_t::acq_t acq(&parent->ack_lock);
    std::map<peer_id_t, directory_echo_version_t>::iterator it = parent->last_acked.find(peer);
    if (it != parent->last_acked.end() && it->second >= _version) {
        pulse();
        return;
    }
    typename std::map<peer_id_t, std::multimap<directory_echo_version_t, ack_waiter_t *> >::iterator it2 =
        parent->waiters.find(peer);
    if (it2 == parent->waiters.end()) {
        it2 = parent->waiters.insert(std::make_pair(peer, std::multimap<directory_echo_version_t, ack_waiter_t *>())).first;
    }
    map_entry.init(new multimap_insertion_sentry_t<directory_echo_version_t, ack_waiter_t *>(&it2->second, _version, this));
}

template<class internal_t>
directory_echo_writer_t<internal_t>::directory_echo_writer_t(
        mailbox_manager_t *mm,
        const internal_t &initial) :
    ack_mailbox(mm, std::bind(&directory_echo_writer_t<internal_t>::on_ack, this, ph::_1, ph::_2, ph::_3)),
    value_watchable(directory_echo_wrapper_t<internal_t>(initial, 0, ack_mailbox.get_address())),
    version(0)
    { }

template<class internal_t>
void directory_echo_writer_t<internal_t>::on_ack(
        UNUSED signal_t *interruptor, 
        peer_id_t peer,
        directory_echo_version_t _version) {
    DEBUG_VAR mutex_assertion_t::acq_t acq(&ack_lock);
    std::map<peer_id_t, directory_echo_version_t>::iterator it = last_acked.find(peer);
    if (it == last_acked.end() || it->second < _version) {
        last_acked[peer] = _version;
    }
    typename std::map<peer_id_t, std::multimap<directory_echo_version_t, ack_waiter_t *> >::iterator it2 =
        waiters.find(peer);
    if (it2 != waiters.end()) {
        for (typename std::multimap<directory_echo_version_t, ack_waiter_t *>::iterator it3 = it2->second.begin();
                it3 != it2->second.upper_bound(_version); it3++) {
            if (!it3->second->is_pulsed()) {
                it3->second->pulse();
            }
        }
    }
}

template<class internal_t>
directory_echo_mirror_t<internal_t>::directory_echo_mirror_t(
        mailbox_manager_t *mm,
        watchable_map_t<peer_id_t, directory_echo_wrapper_t<internal_t> > *p) :
    watchable_map_transform_t<peer_id_t, directory_echo_wrapper_t<internal_t>,
                              peer_id_t, internal_t>(p),
    mailbox_manager(mm), peers(p),
    subs(p,
        [this](const peer_id_t &peer,
               const directory_echo_wrapper_t<internal_t> *wrapper) {
            this->on_change(peer, wrapper);
        })
    { }

template<class internal_t>
void directory_echo_mirror_t<internal_t>::on_change(
        const peer_id_t &peer, const directory_echo_wrapper_t<internal_t> *wrapper) {
    if (wrapper != nullptr) {
        int version = wrapper->version;
        auto it = last_seen.find(peer);
        if (it == last_seen.end() || it->second < version) {
            last_seen[peer] = version;
            /* It's important that `ack_version()` is not run until after the
            `watchable_map_t` delivers all of its changeds. That guarantees that whatever
            is watching our `watchable_map_t` will see the change before we tell the
            other peer that we saw the change. */
            coro_t::spawn_sometime(std::bind(
                &directory_echo_mirror_t<internal_t>::ack_version, this,
                wrapper->ack_mailbox, version, auto_drainer_t::lock_t(&drainer)
                ));
        }
    } else {
        /* Erase `_last_seen` table entries for now-disconnected peers. This serves two
        purposes:
        1. It saves space if many peers connect and disconnect (this is not very
            important, but nice theoretically)
        2. It means that if they re-connect, we will re-transmit the ack. This is
            important because maybe they didn't get the ack the first time due to
            the connection going down.
        TODO: There's a race condition here. It's possible that if a peer disconnects and
        reconnects quickly, the ack message will get lost but we won't notice it,
        because `watchable_map_t` won't necessarily deliver every single change. I don't
        think it can happen in practice, but we shouldn't rely on this behavior. But
        `directory_echo_mirror_t` will be removed when we implement Raft, so I'm not
        going to bother fixing this race condition. */
        last_seen.erase(peer);
    }
}

template<class internal_t>
void directory_echo_mirror_t<internal_t>::ack_version(mailbox_t<void(peer_id_t, directory_echo_version_t)>::address_t peer, directory_echo_version_t version, auto_drainer_t::lock_t) {
    send(mailbox_manager, peer,
        mailbox_manager->get_connectivity_cluster()->get_me(), version);
}

#include "containers/cow_ptr.hpp"
#include "clustering/reactor/metadata.hpp"

template class directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> >;
template class directory_echo_writer_t<cow_ptr_t<reactor_business_card_t> >;
template class directory_echo_mirror_t<cow_ptr_t<reactor_business_card_t> >;

