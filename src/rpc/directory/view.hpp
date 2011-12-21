#ifndef __RPC_DIRECTORY_VIEW_HPP__
#define __RPC_DIRECTORY_VIEW_HPP__

#include "concurrency/pubsub.hpp"
#include "rpc/connectivity/connectivity.hpp"

class directory_service_t :
        public home_thread_mixin_t
{
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
        peer_value_freeze_t(directory_service_t *, peer_id_t) THROWS_NOTHING;
        void assert_is_holding(directory_service_t *, peer_id_t) THROWS_NOTHING;
    private:
        directory_service_t *directory;
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
            directory_t *, peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING;
        void reset() THROWS_NOTHING;
        void reset(directory_service_t *, peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING;
    private:
        publisher_t<
                boost::function<void()>
                >::subscription_t subs;
    };

    /* `our_value_lock_acq_t` prevents anyone from changing our directory-value
    other than the person who holds the `our_value_lock_acq_t`. Its constructor
    may block. It can only be acquired on the `directory_service_t`'s home
    thread. It doesn't interfere with `peer_value_freeze_t`, but be sure not to
    call `set_our_value()` while a `peer_value_freeze_t` exists in the same
    coroutine. */
    class our_value_lock_acq_t {
    public:
        our_value_lock_acq_t(directory_service_t *) THROWS_NOTHING;
        void assert_is_holding(directory_service_t *) THROWS_NOTHING;
    private:
        mutex_t::acq_t acq;
    };

    /* Returns the `connectivity_service_t` associated with the directory
    service. The directory should report a non-nothing value for a peer if and
    only if the `connectivity_service_t` says that the peer is connected. */
    virtual connectivity_service_t *get_connectivity() = 0;

protected:
    virtual ~directory_service_t() THROWS_NOTHING { }

private:
    friend class peer_value_freeze_t;
    friend class peer_value_subscription_t;

    /* Returns a lock and associated publisher for the value of a given peer.
    These should be per-thread. */
    virtual mutex_assertion_t *get_peer_value_lock(peer_id_t) THROWS_NOTHING = 0;
    virtual publisher_t<
        boost::function<void()>
        > *get_peer_value_publisher(peer_id_t, peer_value_lock_acq_t *proof) THROWS_NOTHING = 0;

    /* Returns a lock for writing to our value. `get_our_value_lock()` should
    only be accessed on the `directory_service_t`'s home thread. */
    virtual mutex_t *get_our_value_lock() THROWS_NOTHING = 0;
};

/* `directory_rview_t` and `directory_rwview_t` refer to the same part of the
directory metadata on each peer. */

template<class metadata_t>
class directory_rview_t {
public:
    /* Returns the current value of this view of the given peer's directory
    metadata. If the peer is not connected, returns nothing. */
    virtual boost::optional<metadata_t> get_value(peer_id_t peer) THROWS_NOTHING = 0;

    /* Returns the directory that this is a part of. */
    virtual directory_service_t *get_directory() THROWS_NOTHING = 0;

protected:
    virtual ~directory_rview_t() THROWS_NOTHING { }
};

template<class metadata_t>
class directory_rwview_t : public directory_rview_t<metadata_t> {
public:
    /* Changes the value of this part of the directory for the peer that is us.
    You must hold a `directory_service_t::our_value_lock_acq_t`; this is to
    avoid race conditions with two different things changing the directory
    metadata simultaneously. `set_our_value()` may block. It can only be called
    on the home thread of the `directory_service_t`.

    Don't call `set_our_value()` when you hold a
    `directory_service_t::peer_value_freeze_t`, because that will crash. But
    it's OK to call `set_our_value()` while other people are creating and
    destroying `peer_value_freeze_t`s, because they shouldn't block while
    holding them anyway. */
    virtual void set_our_value(const metadata_t &new_value_for_us, directory_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING = 0;
};

/* `directory_peer_rview_t` refers to a certain part of the directory metadata
on a specific peer. */

template<class metadata_t>
class directory_peer_rview_t {
public:
    /* Return the current value of this view of `get_peer()`'s directory
    metadata. If `get_peer()` is not connected, returns nothing. */
    virtual boost::optional<metadata_t> get_value() THROWS_NOTHING = 0;

    /* Returns the directory that this is a part of. */
    virtual directory_service_t *get_directory() THROWS_NOTHING = 0;

    /* Returns the peer that this `directory_peer_rview_t` is looking at. */
    virtual peer_id_t get_peer() THROWS_NOTHING = 0;

protected:
    virtual ~directory_peer_rview_t() THROWS_NOTHING { }
};

#endif /* __RPC_DIRECTORY_VIEW_HPP__ */
