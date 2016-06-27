// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_SEMILATTICE_SEMILATTICE_MANAGER_HPP_
#define RPC_SEMILATTICE_SEMILATTICE_MANAGER_HPP_

#include <map>
#include <utility>

#include "rpc/mailbox/mailbox.hpp"
#include "rpc/semilattice/view.hpp"

class cond_t;
template <class> class promise_t;

/* `semilattice_manager_t` runs on top of a `message_service_t` and synchronizes
a value, called the "global semilattice metadata", between all of the nodes in
the cluster. `semilattice_manager_t` is templatized on the type of the global
semilattice metadata. The type `metadata_t` must satisfy the following
constraints:

1. It must have public and sane default constructor, copy constructor,
    copy assignment, and destructor.

2. It must be serializable.

3. There must exist a function:

        void semilattice_join(metadata_t *a, const metadata_t &b);

    such that `metadata_t` is a semilattice and `semilattice_join(a, b)` sets
    `*a` to the semilattice-join of `*a` and `b`.

Currently it's not thread-safe at all; all accesses to the metadata must be on
the home thread of the `semilattice_manager_t`. */

template<class metadata_t>
class semilattice_manager_t :
    public home_thread_mixin_t,
    public cluster_message_handler_t
{
public:
    semilattice_manager_t(connectivity_cluster_t *connectivity_cluster,
                          connectivity_cluster_t::message_tag_t message_tag,
                          const metadata_t &initial_metadata);
    ~semilattice_manager_t() THROWS_NOTHING;

    boost::shared_ptr<semilattice_readwrite_view_t<metadata_t> > get_root_view();

private:
    typedef uint64_t metadata_version_t, sync_from_query_id_t, sync_to_query_id_t;

    /* `get_root_view()` returns a pointer to this. It just exists to implement
    `semilattice_readwrite_view_t` for us. */
    class root_view_t : public semilattice_readwrite_view_t<metadata_t> {
    public:
        explicit root_view_t(semilattice_manager_t *);
        semilattice_manager_t *parent;
        metadata_t get();
        void join(const metadata_t &);
        void sync_from(peer_id_t, signal_t *) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t);
        void sync_to(peer_id_t, signal_t *) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t);
        publisher_t<std::function<void()> > *get_publisher();
    };

    class metadata_writer_t;
    class sync_from_query_writer_t;
    class sync_from_reply_writer_t;
    class sync_to_query_writer_t;
    class sync_to_reply_writer_t;

    /* These are called by the `connectivity_cluster_t`. They shouldn't block. */
    void on_message(connectivity_cluster_t::connection_t *, auto_drainer_t::lock_t,
                    read_stream_t *);
    void on_connection_change(
        const peer_id_t &peer_id,
        const connectivity_cluster_t::connection_pair_t *pair);

    void join_metadata_locally(metadata_t);
    void wait_for_version_from_peer(peer_id_t peer, metadata_version_t version, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t);

    const boost::shared_ptr<root_view_t> root_view;

    metadata_version_t metadata_version;
    metadata_t metadata;
    publisher_controller_t<std::function<void()> > metadata_publisher;
    rwi_lock_assertion_t metadata_mutex;

    std::map<peer_id_t, connectivity_cluster_t::connection_pair_t> last_connections;

    std::map<peer_id_t, metadata_version_t> last_versions_seen;
    std::multimap<std::pair<peer_id_t, metadata_version_t>, cond_t *> version_waiters;
    mutex_assertion_t peer_version_mutex;

    sync_from_query_id_t next_sync_from_query_id;
    std::map<sync_from_query_id_t, promise_t<metadata_version_t> *> sync_from_waiters;

    sync_to_query_id_t next_sync_to_query_id;
    std::map<sync_to_query_id_t, cond_t *> sync_to_waiters;

    /* Any time we want to send a message over the network, we acquire this semaphore
    first. */
    new_semaphore_t semaphore;

    /* Destructor order is important here. First we destroy the
    `connection_change_subscription`, so that we don't spawn any more coroutines. (We
    rely on the code that constructed us to make sure to delete the
    `connectivity_cluster_t::run_t` before deleting us, so `on_message()` won't get
    called.) Then we destroy `drainers`, which blocks until all of the coroutines are
    done. Only once all the coroutines are done is it safe to desstroy the member
    variables. */

    one_per_thread_t<auto_drainer_t> drainers;

    watchable_map_t<peer_id_t, connectivity_cluster_t::connection_pair_t>::all_subs_t
        connection_change_subscription;
};

#endif /* RPC_SEMILATTICE_SEMILATTICE_MANAGER_HPP_ */
