#include "errors.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/make_shared.hpp>

#include "concurrency/pmap.hpp"
#include "concurrency/wait_any.hpp"

template<class metadata_t>
semilattice_manager_t<metadata_t>::semilattice_manager_t(message_service_t *ms, const metadata_t &initial_metadata) :
    message_service(ms),
    root_view(boost::make_shared<root_view_t>(this)),
    metadata(initial_metadata),
    event_watcher(
        boost::bind(&semilattice_manager_t<metadata_t>::on_connect, this, _1),
        boost::bind(&semilattice_manager_t<metadata_t>::on_disconnect, this, _1)),
    ping_id_counter(0)
{
    ASSERT_FINITE_CORO_WAITING;
    connectivity_service_t::peers_list_freeze_t freeze(message_service->get_connectivity_service());
    rassert(message_service->get_connectivity_service()->get_peers_list().empty());
    event_watcher.reset(message_service->get_connectivity_service(), &freeze);
}

template<class metadata_t>
semilattice_manager_t<metadata_t>::~semilattice_manager_t() THROWS_NOTHING {
    assert_thread();
    root_view->parent = NULL;
}

template<class metadata_t>
boost::shared_ptr<semilattice_readwrite_view_t<metadata_t> > semilattice_manager_t<metadata_t>::get_root_view() {
    assert_thread();
    return root_view;
}

template<class metadata_t>
semilattice_manager_t<metadata_t>::root_view_t::root_view_t(semilattice_manager_t *p)
    : parent(p) {
    parent->assert_thread();
}

template<class metadata_t>
metadata_t semilattice_manager_t<metadata_t>::root_view_t::get() {
    rassert(parent, "accessing `semilattice_manager_t` root view when cluster "
        "no longer exists");
    parent->assert_thread();
    return parent->metadata;
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::root_view_t::join(const metadata_t &added_metadata) {
    rassert(parent, "accessing `semilattice_manager_t` root view when cluster "
        "no longer exists");
    parent->assert_thread();

    parent->join_metadata_locally(added_metadata);

    /* Distribute changes to all peers we can currently see. If we can't
    currently see a peer, that's OK; it will hear about the metadata change when
    it reconnects, via the `semilattice_manager_t`'s `on_connect()` handler. */
    connectivity_service_t::peers_list_freeze_t freeze(parent->message_service->get_connectivity_service());
    std::set<peer_id_t> peers = parent->message_service->get_connectivity_service()->get_peers_list();
    for (std::set<peer_id_t>::iterator it = peers.begin(); it != peers.end(); it++) {
        if (*it != parent->message_service->get_connectivity_service()->get_me()) {
            coro_t::spawn_sometime(boost::bind(
                &semilattice_manager_t<metadata_t>::send_metadata_to_peer, parent,
                *it, added_metadata, auto_drainer_t::lock_t(parent->drainers.get())
                ));
        }
    }
}

/* TODO: This system of implementing `sync_from()` assumes that the network
layer does not reorder messages. For now that's true, but we shouldn't rely on
it in the future. `sync_from()` and `sync_to()` kind of just suck... */

template<class metadata_t>
void semilattice_manager_t<metadata_t>::root_view_t::sync_from(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
    rassert(parent, "accessing `semilattice_manager_t` root view when cluster "
        "no longer exists");
    parent->assert_thread();
    int ping_id = parent->ping_id_counter++;
    cond_t response_cond;
    map_insertion_sentry_t<int, cond_t *> response_listener(&parent->ping_waiters, ping_id, &response_cond);
    disconnect_watcher_t watcher(parent->message_service->get_connectivity_service(), peer);
    parent->message_service->send_message(peer,
        boost::bind(&semilattice_manager_t<metadata_t>::write_ping, _1, ping_id));
    wait_any_t waiter(&response_cond, &watcher);
    wait_interruptible(&waiter, interruptor);   /* May throw `interrupted_exc_t` */
    if (watcher.is_pulsed()) {
        throw sync_failed_exc_t();
    }
    rassert(response_cond.is_pulsed());
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::root_view_t::sync_to(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
    parent->assert_thread();
    /* For now we just implement `sync_to()` the same way we implement
    `sync_from()`: we ping the peer. In the future, it could send an ack every
    time the metadata changes and we could just check how recently the last ack
    was. */
    sync_from(peer, interruptor);
}

template<class metadata_t>
publisher_t<boost::function<void()> > *semilattice_manager_t<metadata_t>::root_view_t::get_publisher() {
    rassert(parent, "accessing `semilattice_manager_t` root view when cluster no "
        "longer exists");
    parent->assert_thread();
    return parent->metadata_publisher.get_publisher();
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::write_metadata(std::ostream &stream, metadata_t md) {
    // We must be on the connection thread.
    stream << 'M';
    boost::archive::binary_oarchive archive(stream);
    archive << md;
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::write_ping(std::ostream &stream, int ping_id) {
    // We must be on the connection thread.
    stream << 'P';
    stream.write(reinterpret_cast<const char *>(&ping_id), sizeof(ping_id));
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::write_ping_response(std::ostream &stream, int ping_id) {
    // We must be on the connection thread
    stream << 'R';
    stream.write(reinterpret_cast<const char *>(&ping_id), sizeof(ping_id));
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::on_message(peer_id_t sender, std::istream &stream) {
    char code;
    stream >> code;

    // TODO: Hard-coded constants.
    if (code == 'M') {
        metadata_t added_metadata;
        {
            boost::archive::binary_iarchive archive(stream);
            archive >> added_metadata;
        }
        coro_t::spawn_sometime(boost::bind(
            &semilattice_manager_t<metadata_t>::join_metadata_locally_on_home_thread, this,
            added_metadata, auto_drainer_t::lock_t(drainers.get())
            ));

    } else if (code == 'P') {
        // Good news: We never leave the connection thread.
        // TODO: We should not be sending an int over the wire.
        int ping_id;
        stream.read(reinterpret_cast<char *>(&ping_id), sizeof(ping_id));
        coro_t::spawn_sometime(boost::bind(
            &semilattice_manager_t<metadata_t>::send_ping_response_to_peer, this,
            sender, ping_id, auto_drainer_t::lock_t(drainers.get())
            ));

    } else if (code == 'R') {
        int ping_id;
        stream.read(reinterpret_cast<char *>(&ping_id), sizeof(ping_id));
        coro_t::spawn_sometime(boost::bind(
            &semilattice_manager_t<metadata_t>::release_ping_waiter_on_home_thread, this,
            ping_id, auto_drainer_t::lock_t(drainers.get())
            ));

    } else {
        /* We don't try to tolerate garbage on the wire. The network layer had
        better not corrupt our messages. If we need to, we'll add more
        checksums. */
        crash("Unexpected utility message code: %d", (int)code);
    }
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::on_connect(peer_id_t peer) {
    assert_thread();

    /* We have to spawn this in a separate coroutine because `on_connect()` is
    not supposed to block. */
    coro_t::spawn_sometime(boost::bind(
        &semilattice_manager_t<metadata_t>::send_metadata_to_peer, this,
        peer, metadata, auto_drainer_t::lock_t(drainers.get())
        ));
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::on_disconnect(peer_id_t) {
    assert_thread();
    /* Ignore event */
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::send_metadata_to_peer(peer_id_t peer, metadata_t m, auto_drainer_t::lock_t) {
    message_service->send_message(peer,
        boost::bind(&semilattice_manager_t<metadata_t>::write_metadata, _1, m));
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::send_ping_response_to_peer(peer_id_t peer, int ping_id, auto_drainer_t::lock_t) {
    message_service->send_message(peer,
        boost::bind(&semilattice_manager_t<metadata_t>::write_ping_response, _1, ping_id));
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::join_metadata_locally_on_home_thread(metadata_t md, auto_drainer_t::lock_t) {
    on_thread_t thread_switcher(home_thread());
    join_metadata_locally(md);
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::release_ping_waiter_on_home_thread(int ping_id, auto_drainer_t::lock_t) {
    on_thread_t thread_switcher(home_thread());
    std::map<int, cond_t *>::iterator it = ping_waiters.find(ping_id);
    if (it != ping_waiters.end()) {
        (*it).second->pulse();
    }
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::call_function_with_no_args(const boost::function<void()> &fun) {
    fun();
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::join_metadata_locally(metadata_t added_metadata) {
    assert_thread();
    rwi_lock_assertion_t::write_acq_t acq(&metadata_mutex);
    semilattice_join(&metadata, added_metadata);
    metadata_publisher.publish(&semilattice_manager_t<metadata_t>::call_function_with_no_args);
}
