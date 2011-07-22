#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

template<class metadata_t>
metadata_cluster_t<metadata_t>::metadata_cluster_t(int port, const metadata_t &initial_metadata) :
    mailbox_cluster_t(port),
    /* Watch ourself for new peers connecting */
    event_watcher_t(this),
    metadata(get_num_threads(), initial_metadata)
    { }

template<class metadata_t>
metadata_cluster_t<metadata_t>::~metadata_cluster_t() {
}

template<class metadata_t>
metadata_t metadata_cluster_t<metadata_t>::get_metadata() {
    return metadata[get_thread_id()];
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::join_metadata(metadata_t added_metadata) {

    /* Distribute changes to metadata to all local threads.
    TODO: This blocks. Can/should we do something about that? */
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
    pmap(get_num_threads(), boost::bind(
        &metadata_cluster_t<metadata_t>::join_metadata_on_thread,
        this,
        _1,
        &added_metadata
        ));
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::join_metadata_on_thread(int thread, const metadata_t *added_metadata) {
    on_thread_t thread_switcher(thread);
    semilattice_join(&metadata[thread], *added_metadata);
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::write_metadata(std::ostream &stream, metadata_t md) {
    boost::archive::text_oarchive archive(stream);
    archive << md;
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::on_utility_message(peer_id_t, std::istream &stream, boost::function<void()> &on_done) {
    boost::archive::text_iarchive archive(stream);
    metadata_t added_metadata;
    archive >> added_metadata;
    on_done();
    join_metadata_locally(added_metadata);
}

template<class metadata_t>
void metadata_cluster_t<metadata_t>::on_connect(peer_id_t peer) {
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
