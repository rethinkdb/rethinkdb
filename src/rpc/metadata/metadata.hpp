#ifndef __RPC_METADATA_METADATA_HPP__
#define __RPC_METADATA_METADATA_HPP__

#include "rpc/mailbox/mailbox.hpp"

/* Forward declarations */

template<class metadata_t>
struct metadata_watcher_t;

template<class metadata_t>
struct metadata_view_t;

/* `metadata_view_t` is a reference to some subset of the metadata for a
cluster. Components that need to interact with metadata should take a
`metadata_view_t<metadata_t> *`, where `metadata_t` is the type of the piece of
metadata that they need to use. That way, the individual components don't need
to know the type of the overall cluster-wide metadata. */

template<class metadata_t>
class metadata_view_t {

public:
    /* Get the current metadata. TODO: This makes a copy. We could replace
    `get_metadata()` with some sort of COW-based solution if this becomes a
    performance issue. */
    virtual metadata_t get_metadata() = 0;

    /* Join the given metadata with the current metadata. */
    virtual void join_metadata(metadata_t) = 0;

protected:
    virtual ~metadata_view_t() { }

private:
    friend class metadata_watcher_t<metadata_t>;
    virtual publisher_t<boost::function<void()> > *get_publisher() = 0;
};

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
class metadata_cluster_t :
    public mailbox_cluster_t,
    private event_watcher_t
{
public:
    metadata_cluster_t(int port, const metadata_t &initial_metadata);
    ~metadata_cluster_t();

    metadata_view_t<metadata_t> *get_root_view();

private:
    /* `get_root_view()` returns a pointer to this. It just exists to implement
    `metadata_view_t` for us. */
    struct root_view_t : public metadata_view_t<metadata_t> {
        root_view_t(metadata_cluster_t *);
        metadata_cluster_t *parent;
        metadata_t get_metadata();
        void join_metadata(metadata_t);
        publisher_t<boost::function<void()> > *get_publisher();
    } root_view;

    metadata_t metadata;

    void join_metadata_locally(metadata_t);

    /* Infrastructure for notifying things when metadata changes */
    static void call(boost::function<void()>);
    public_publisher_t<boost::function<void()> > change_publisher;

    static void write_metadata(std::ostream&, metadata_t);
    void on_utility_message(peer_id_t, std::istream&, boost::function<void()>&);
    void on_connect(peer_id_t);
    void on_disconnect(peer_id_t);
};

/* Use `metadata_watcher_t` to be notified when metadata changes. */
template<class metadata_t>
struct metadata_watcher_t {
    metadata_watcher_t(boost::function<void()>, metadata_view_t<metadata_t> *);
private:
    publisher_t<boost::function<void()> >::subscription_t subs;
    DISABLE_COPYING(metadata_watcher_t);
};

#include "rpc/metadata/metadata.tcc"

#endif /* __RPC_METADATA_METADATA_HPP__ */
