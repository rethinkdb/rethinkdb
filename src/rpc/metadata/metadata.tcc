#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include "concurrency/pmap.hpp"

template<class metadata_t>
metadata_cluster_t<metadata_t>::metadata_cluster_t(int port, const metadata_t &initial_metadata) :
    mailbox_cluster_t(port),
    /* Watch ourself for new peers connecting */
    event_watcher_t(this),
    metadata(initial_metadata)
    { }

template<class metadata_t>
metadata_cluster_t<metadata_t>::~metadata_cluster_t() {
}

template<class metadata_t>
metadata_t metadata_cluster_t<metadata_t>::get_metadata() {
    assert_thread();
    return metadata;
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::join_metadata(metadata_t added_metadata) {
    assert_thread();

    /* Make change on ourself */
    join_metadata_locally(added_metadata);

    /* Distribute changes to all peers we can currently see. If we can't
    currently see a peer, that's OK; it will hear about the metadata change when
    it reconnects, via our `on_connect()` handler. */
    std::map<peer_id_t, peer_address_t> peers = get_everybody();
    for (std::map<peer_id_t, peer_address_t>::iterator it = peers.begin(); it != peers.end(); it++) {
        peer_id_t peer = (*it).first;
        if (peer != get_me()) {
            send_utility_message(peer,
                boost::bind(&metadata_cluster_t<metadata_t>::write_metadata, _1, added_metadata));
        }
    }
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::join_metadata_locally(metadata_t added_metadata) {
    assert_thread();

    semilattice_join(&metadata, added_metadata);

    change_publisher.publish(&metadata_cluster_t<metadata_t>::call);
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::write_metadata(std::ostream &stream, metadata_t md) {
    boost::archive::binary_oarchive archive(stream);
    archive << md;
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::on_utility_message(peer_id_t, std::istream &stream, boost::function<void()> &on_done) {
    assert_thread();
    metadata_t added_metadata;
    {
        boost::archive::binary_iarchive archive(stream);
        archive >> added_metadata;
    }
    on_done();
    join_metadata_locally(added_metadata);
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
            boost::bind(&metadata_cluster_t<metadata_t>::write_metadata, _1, get_metadata())
            )
        ));
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::on_disconnect(peer_id_t) {
    /* Ignore event */
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::call(boost::function<void()> fun) {
    fun();
}

template<class metadata_t>
metadata_watcher_t<metadata_t>::metadata_watcher_t(boost::function<void()> cb, metadata_cluster_t<metadata_t> *cl) :
    subs(cb, &cl->change_publisher) { }

