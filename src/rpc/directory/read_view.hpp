#ifndef __RPC_DIRECTORY_READ_VIEW_HPP__
#define __RPC_DIRECTORY_READ_VIEW_HPP__

#include "concurrency/pubsub.hpp"
#include "rpc/connectivity/connectivity.hpp"

class directory_read_service_t {
public:
    /* `peer_value_freeze_t` prevents value-change notifications from being
    delivered for the given peer. It's useful so that you can atomically read
    the current value for that peer and construct a `peer_value_subscription_t`
    without worrying about the value changing in between. Don't block while it
    exists.

    Underneath the hood, `peer_value_freeze_t` is just asserting that no changes
    happen. That's why it's so dangerous to block while it exists. */
    class peer_value_freeze_t {
    public:
        peer_value_freeze_t(directory_read_service_t *, peer_id_t) THROWS_NOTHING;
        void assert_is_holding(directory_read_service_t *, peer_id_t) THROWS_NOTHING;
    private:
        directory_read_service_t *directory;
        peer_id_t peer;
        mutex_assertion_t::acq_t acq;
    };

    /* `peer_value_subscription_t` calls the given function whenever the given
    peer changes its directory metadata. */
    class peer_value_subscription_t {
    public:
        peer_value_subscription_t(
            const boost::function<void()> &change_cb) THROWS_NOTHING;
        peer_value_subscription_t(
            const boost::function<void()> &change_cb,
            directory_read_service_t *, peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING;
        void reset() THROWS_NOTHING;
        void reset(directory_read_service_t *, peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING;
    private:
        publisher_t<
                boost::function<void()>
                >::subscription_t subs;
    };

    /* Returns the `connectivity_service_t` associated with the directory
    service. The directory should report a non-nothing value for a peer if and
    only if the `connectivity_service_t` says that the peer is connected. */
    virtual connectivity_service_t *get_connectivity() = 0;

protected:
    virtual ~directory_read_service_t() THROWS_NOTHING { }

private:
    friend class peer_value_freeze_t;
    friend class peer_value_subscription_t;

    /* Returns a lock and associated publisher for the value of a given peer.
    These should be per-thread. */
    virtual mutex_assertion_t *get_peer_value_lock(peer_id_t) THROWS_NOTHING = 0;
    virtual publisher_t<
        boost::function<void()>
        > *get_peer_value_publisher(peer_id_t, peer_value_lock_acq_t *proof) THROWS_NOTHING = 0;
};

template<class metadata_t>
class directory_single_rview_t {
public:
    virtual ~directory_single_rview_t() THROWS_NOTHING { }
    virtual directory_single_rview_t *clone() THROWS_NOTHING = 0;

    /* Return the current value of this view of `get_peer()`'s directory
    metadata. If `get_peer()` is not connected, returns nothing. */
    virtual boost::optional<metadata_t> get_value() THROWS_NOTHING = 0;

    /* Returns the directory service that this is a part of. */
    virtual directory_read_service_t *get_directory_service() THROWS_NOTHING = 0;

    /* Returns the peer that this `directory_peer_rview_t` is looking at. */
    virtual peer_id_t get_peer() THROWS_NOTHING = 0;

    /* Constructs a sub-view that looks at some sub-component of this view. */
    template<class inner_t>
    clone_ptr_t<directory_single_rview_t<inner_t> > subview(const clone_ptr_t<read_lens_t<inner_t, metadata_t> > &lens) THROWS_NOTHING;
};

template<class metadata_t>
class directory_rview_t {
public:
    virtual ~directory_rview_t() THROWS_NOTHING { }
    virtual directory_rview_t *clone() THROWS_NOTHING = 0;

    /* Returns the current value of this view of the given peer's directory
    metadata. If the peer is not connected, returns nothing. */
    virtual boost::optional<metadata_t> get_value(peer_id_t peer) THROWS_NOTHING = 0;

    /* Returns the directory that this is a part of. */
    virtual directory_read_service_t *get_directory() THROWS_NOTHING = 0;

    /* Constructs a sub-view of this view, that looks at some sub-component of
    all of the peers simultaneously. */
    template<class inner_t>
    clone_ptr_t<directory_rview_t<inner_t> > subview(const clone_ptr<read_lens_t<inner_t, metadata_t> > &lens) THROWS_NOTHING;

    /* Constructs a sub-view of this view, that looks at the same object but at
    only a single one of the peers. */
    clone_ptr_t<directory_single_rview_t<metadata_t> > subview(peer_id_t) THROWS_NOTHING;
};

#include "rpc/directory/read_view.tcc"

#endif /* __RPC_DIRECTORY_READ_VIEW_HPP__ */
