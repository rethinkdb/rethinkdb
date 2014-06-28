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
    parent->value_watchable.set_value(directory_echo_wrapper_t<internal_t>(buffer,
                                                                           parent->version,
                                                                           parent->ack_mailbox.get_address()));
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
    ack_mailbox(mm, std::bind(&directory_echo_writer_t<internal_t>::on_ack, this, ph::_1, ph::_2, auto_drainer_t::lock_t(&drainer))),
    value_watchable(directory_echo_wrapper_t<internal_t>(initial, 0, ack_mailbox.get_address())),
    version(0)
    { }

template<class internal_t>
void directory_echo_writer_t<internal_t>::on_ack(peer_id_t peer, directory_echo_version_t _version, auto_drainer_t::lock_t lock) {
    lock.assert_is_holding(&drainer);
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
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, directory_echo_wrapper_t<internal_t> > > > &p) :
    mailbox_manager(mm), peers(p),
    subview(change_tracking_map_t<peer_id_t, internal_t>()),
    sub(std::bind(&directory_echo_mirror_t::on_change, this)) {

    typename watchable_t<change_tracking_map_t<peer_id_t, directory_echo_wrapper_t<internal_t> > >::freeze_t freeze(peers);
    sub.reset(peers, &freeze);
    on_change();
}

template<class internal_t>
void directory_echo_mirror_t<internal_t>::on_change() {
    ASSERT_FINITE_CORO_WAITING;

    bool anything_changed = false;

    struct op_closure_t {
        static void apply(
                std::map<peer_id_t, directory_echo_version_t> *_last_seen,
                change_tracking_map_t<peer_id_t, internal_t> *_subview_value,
                auto_drainer_t *_drainer,
                bool *_anything_changed,
                directory_echo_mirror_t<internal_t> *parent,
                const change_tracking_map_t<peer_id_t, directory_echo_wrapper_t<internal_t> > *snapshot) {
            for (auto it = snapshot->get_inner().begin(); it != snapshot->get_inner().end(); it++) {
                int version = it->second.version;
                auto jt = _last_seen->find(it->first);
                if (jt == _last_seen->end() || jt->second < version) {
                    (*_last_seen)[it->first] = version;
                    /* Because `spawn_sometime()` won't run its function until after
                    `on_change()` has returned, we will call `subview.set_value()`
                    before the acks are sent. That guarantees that whatever is watching
                    our subview will see the change before we tell the other peer that
                    we saw the change. */
                    coro_t::spawn_sometime(std::bind(
                        &directory_echo_mirror_t<internal_t>::ack_version, parent,
                        it->second.ack_mailbox, version,
                        auto_drainer_t::lock_t(_drainer)));
                    if (!*_anything_changed) {
                        _subview_value->begin_version();
                        *_anything_changed = true;
                    }
                    _subview_value->set_value(it->first, it->second.internal);
                }
            }
            /* Erase `_last_seen` table entries for now-disconnected peers. This serves
            two purposes:
            1. It saves space if many peers connect and disconnect (this is not very
                important, but nice theoretically)
            2. It means that if they re-connect, we will re-transmit the ack. This is
                important because maybe they didn't get the ack the first time due to
                the connection going down.
            */
            for (auto it = _last_seen->begin(); it != _last_seen->end();) {
                if (snapshot->get_inner().find(it->first) == snapshot->get_inner().end()) {
                    if (!*_anything_changed) {
                        _subview_value->begin_version();
                        *_anything_changed = true;
                    }
                    _subview_value->delete_value(it->first);
                    _last_seen->erase(it++);
                } else {
                    ++it;
                }
            }
        }
    };

    peers->apply_read(std::bind(&op_closure_t::apply,
                                &last_seen,
                                &subview_value,
                                &drainer,
                                &anything_changed,
                                this,
                                ph::_1));

    /* If nothing actually changed, don't bother sending out a spurious update
    to our sub-listeners. */
    if (anything_changed) {
        subview.set_value(subview_value);
    }
}

template<class internal_t>
void directory_echo_mirror_t<internal_t>::ack_version(mailbox_t<void(peer_id_t, directory_echo_version_t)>::address_t peer, directory_echo_version_t version, auto_drainer_t::lock_t) {
    send(mailbox_manager, peer, mailbox_manager->get_connectivity_service()->get_me(), version);
}

#include "containers/cow_ptr.hpp"
#include "clustering/reactor/metadata.hpp"

template class directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> >;
template class directory_echo_writer_t<cow_ptr_t<reactor_business_card_t> >;
template class directory_echo_mirror_t<cow_ptr_t<reactor_business_card_t> >;

#include <string>  // NOLINT(build/include_order)

template class directory_echo_wrapper_t<std::string>;
template class directory_echo_writer_t<std::string>;
template class directory_echo_mirror_t<std::string>;
