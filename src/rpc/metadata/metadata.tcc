#include "errors.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/make_shared.hpp>

#include "concurrency/pmap.hpp"
#include "concurrency/wait_any.hpp"

template<class metadata_t>
metadata_cluster_t<metadata_t>::metadata_cluster_t(int port, const metadata_t &initial_metadata) :
    mailbox_cluster_t(port),
    root_view(boost::make_shared<root_view_t>(this)),
    metadata(initial_metadata),
    event_watcher(
        boost::bind(&metadata_cluster_t<metadata_t>::on_connect, this, _1),
        boost::bind(&metadata_cluster_t<metadata_t>::on_disconnect, this, _1)),
    ping_id_counter(0)
{
    connectivity_service_t::peers_list_freeze_t freeze(this);
    rassert(get_peers_list().size() == 1);
    event_watcher.reset(this, &freeze);
}

template<class metadata_t>
metadata_cluster_t<metadata_t>::~metadata_cluster_t() {
    assert_thread();
    root_view->parent = NULL;
}

template<class metadata_t>
boost::shared_ptr<metadata_readwrite_view_t<metadata_t> > metadata_cluster_t<metadata_t>::get_root_view() {
    assert_thread();
    return root_view;
}

template<class metadata_t>
metadata_cluster_t<metadata_t>::root_view_t::root_view_t(metadata_cluster_t *p)
    : parent(p) {
    parent->assert_thread();
}

template<class metadata_t>
metadata_t metadata_cluster_t<metadata_t>::root_view_t::get() {
    rassert(parent, "accessing `metadata_cluster_t` root view when cluster no "
        "longer exists");
    parent->assert_thread();
    return parent->metadata;
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::root_view_t::join(const metadata_t &added_metadata) {
    rassert(parent, "accessing `metadata_cluster_t` root view when cluster no "
        "longer exists");
    parent->assert_thread();

    parent->join_metadata_locally(added_metadata);

    /* Distribute changes to all peers we can currently see. If we can't
    currently see a peer, that's OK; it will hear about the metadata change when
    it reconnects, via the `metadata_cluster_t`'s `on_connect()` handler. */
    std::set<peer_id_t> peers = parent->get_peers_list();
    for (std::set<peer_id_t>::iterator it = peers.begin(); it != peers.end(); it++) {
        if (*it != parent->get_me()) {
            parent->send_utility_message(*it,
                boost::bind(&metadata_cluster_t<metadata_t>::write_metadata, _1, added_metadata));
        }
    }
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::root_view_t::sync_from(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
    rassert(parent, "accessing `metadata_cluster_t` root view when cluster no "
        "longer exists");
    parent->assert_thread();
    int ping_id = parent->ping_id_counter++;
    cond_t response_cond;
    map_insertion_sentry_t<int, cond_t *> response_listener(&parent->ping_waiters, ping_id, &response_cond);
    disconnect_watcher_t watcher(parent, peer);
    parent->send_utility_message(peer,
        boost::bind(&metadata_cluster_t<metadata_t>::write_ping, _1, ping_id));
    wait_any_t waiter(&response_cond, &watcher);
    wait_interruptible(&waiter, interruptor);
    if (watcher.is_pulsed()) {
        throw sync_failed_exc_t();
    }
    rassert(response_cond.is_pulsed());
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::root_view_t::sync_to(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
    parent->assert_thread();
    /* For now we just implement `sync_to()` the same way we implement
    `sync_from()`: we ping the peer. In the future, it could send an ack every
    time the metadata changes and we could just check how recently the last ack
    was. */
    sync_from(peer, interruptor);
}

template<class metadata_t>
publisher_t<boost::function<void()> > *metadata_cluster_t<metadata_t>::root_view_t::get_publisher() {
    rassert(parent, "accessing `metadata_cluster_t` root view when cluster no "
        "longer exists");
    parent->assert_thread();
    return parent->change_publisher.get_publisher();
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::join_metadata_locally(metadata_t added_metadata) {
    assert_thread();
    mutex_t::acq_t change_acq(&change_mutex);
    semilattice_join(&metadata, added_metadata);
    change_publisher.publish(&metadata_cluster_t<metadata_t>::call);
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::call(boost::function<void()> fun) {
    fun();
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::write_metadata(std::ostream &stream, metadata_t md) {
    // We must be on the connection thread.
    stream << 'M';
    boost::archive::binary_oarchive archive(stream);
    archive << md;
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::write_ping(std::ostream &stream, int ping_id) {
    // We must be on the connection thread.
    stream << 'P';
    stream.write(reinterpret_cast<const char *>(&ping_id), sizeof(ping_id));
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::write_ping_response(std::ostream &stream, int ping_id) {
    // We must be on the connection thread
    stream << 'R';
    stream.write(reinterpret_cast<const char *>(&ping_id), sizeof(ping_id));
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::on_utility_message(peer_id_t sender, std::istream &stream, const boost::function<void()> &on_done) {
    assert_connection_thread(sender);

    char code;
    stream >> code;
    // TODO: Hard-coded constants.
    if (code == 'M') {
        metadata_t added_metadata;
        {
            boost::archive::binary_iarchive archive(stream);
            archive >> added_metadata;
        }
        on_done();
        // TODO: Couldn't this just use one_way_do_on_thread?
        on_thread_t threader(home_thread());
        join_metadata_locally(added_metadata);
    } else if (code == 'P') {
        // Good news: We never leave the connection thread.
        // TODO: We should not be sending an int over the wire.
        int ping_id;
        stream.read(reinterpret_cast<char *>(&ping_id), sizeof(ping_id));
        on_done();
        send_utility_message(sender,
            boost::bind(&metadata_cluster_t<metadata_t>::write_ping_response, _1, ping_id));
    } else if (code == 'R') {
        int ping_id;
        stream.read(reinterpret_cast<char *>(&ping_id), sizeof(ping_id));
        on_done();
        on_thread_t threader(home_thread());
        std::map<int, cond_t *>::iterator it = ping_waiters.find(ping_id);
        if (it != ping_waiters.end()) {
            (*it).second->pulse();
        }
    } else {
        // TODO: Crashing is debatable.
        crash("Unexpected utility message code: %d", (int)code);
    }
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::on_connect(peer_id_t peer) {
    assert_thread();

    /* We have to spawn this in a separate coroutine because `on_connect()` is
    not supposed to block. */
    coro_t::spawn_now(boost::bind(
        &mailbox_cluster_t::send_utility_message,
        this,
        peer,
        boost::function<void(std::ostream&)>(
            boost::bind(&metadata_cluster_t<metadata_t>::write_metadata, _1, metadata)
            )
        ));
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::on_disconnect(peer_id_t) {
    assert_thread();
    /* Ignore event */
}

