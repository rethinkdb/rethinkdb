#ifndef __RPC_METADATA_METADATA_HPP__
#define __RPC_METADATA_METADATA_HPP__

#include "rpc/mailbox/mailbox.hpp"

/* `metadata_cluster_t` is a `mailbox_cluster_t` that uses the utility message
system to synchronize a value, called the "cluster metadata", between all of the
nodes in the cluster. `metadata_cluster_t` is templatized on the type of the
cluster metadata. The type `metadata_t` must satisfy the following constraints:

1. It must have public and sane default constructor, copy constructor,
    copy assignment, and destructor.

2. It must be serializable using `boost::serialization`.

3. There must exist a function:

        void semilattice_join(metadata_t *a, const metadata_t &b);

    such that `metadata_t` is a semilattice and `semilattice_join(a, b)` sets
    `*a` to the semilattice-join of `*a` and `b`.

`metadata_cluster_t` is currently not thread-safe at all. */

template<class metadata_t>
struct metadata_watcher_t;

template<class metadata_t>
class metadata_cluster_t :
    public mailbox_cluster_t,
    private event_watcher_t
{
public:
    metadata_cluster_t(int port, const metadata_t &initial_metadata);
    ~metadata_cluster_t();

    /* Get the current metadata. TODO: This makes a copy. We could replace
    `get_metadata()` with some sort of COW-based solution if this becomes a
    performance issue. */
    metadata_t get_metadata();

    /* Join the given metadata with the current metadata. */
    void join_metadata(metadata_t);

private:
    /* There is one copy of the metadata per thread. This way, reads are very
    fast. */
    metadata_t metadata;

    void join_metadata_locally(metadata_t);

    static void write_metadata(std::ostream&, metadata_t);
    void on_utility_message(peer_id_t, std::istream&, boost::function<void()>&);
    void on_connect(peer_id_t);
    void on_disconnect(peer_id_t);

    /* Infrastructure for notifying things when metadata changes */
    friend class metadata_watcher_t<metadata_t>;
    static void call(boost::function<void()>);
    public_publisher_t<boost::function<void()> > change_publisher;
};

/* Use `metadata_watcher_t` to be notified when metadata changes. */
template<class metadata_t>
struct metadata_watcher_t {
    metadata_watcher_t(boost::function<void()>, metadata_cluster_t<metadata_t> *);
private:
    publisher_t<boost::function<void()> >::subscription_t subs;
    DISABLE_COPYING(metadata_watcher_t);
};

#include "rpc/metadata/metadata.tcc"

#endif /* __RPC_METADATA_METADATA_HPP__ */
