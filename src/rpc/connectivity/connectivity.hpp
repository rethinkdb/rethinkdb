// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_CONNECTIVITY_CONNECTIVITY_HPP_
#define RPC_CONNECTIVITY_CONNECTIVITY_HPP_

#include <set>

#include "arch/address.hpp"
#include "concurrency/mutex.hpp"
#include "concurrency/signal.hpp"
#include "containers/uuid.hpp"
#include "rpc/serialize_macros.hpp"

/* `peer_id_t` is a wrapper around a `uuid_t`. Each newly
created cluster node picks a UUID to be its peer-ID. */
class peer_id_t {
public:
    bool operator==(const peer_id_t &p) const {
        return p.uuid == uuid;
    }
    bool operator!=(const peer_id_t &p) const {
        return p.uuid != uuid;
    }
    bool operator<(const peer_id_t &p) const {
        return p.uuid < uuid;
    }

    peer_id_t()
        : uuid(nil_uuid())
    { }

    explicit peer_id_t(uuid_t u) : uuid(u) { }

    uuid_t get_uuid() const {
        return uuid;
    }

    bool is_nil() const {
        return uuid.is_nil();
    }

private:
    friend class connectivity_cluster_t;

    uuid_t uuid;

    RDB_MAKE_ME_SERIALIZABLE_1(uuid);
};

void debug_print(append_only_printf_buffer_t *buf, const peer_id_t &peer_id);

struct peers_list_callback_t {
public:
    peers_list_callback_t() { }

    virtual void on_connect(peer_id_t peer_id) = 0;
    virtual void on_disconnect(peer_id_t peer_id) = 0;

protected:
    virtual ~peers_list_callback_t() { }

private:
    DISABLE_COPYING(peers_list_callback_t);
};

/* A `connectivity_service_t` is an object that keeps track of peers that are
connected to us. It's an abstract class because there may be multiple types of
connected-ness between us and other peers. For example, we may be in contact
with another peer but have not received their directory yet, in which case the
`(connectivity_service_t *)&the_connectivity_cluster` will say that we are
connected but the `(connectivity_service_t *)&the_directory` will say that we
are not. */

class connectivity_service_t {
public:
    /* Sometimes you want to check the status of a peer or peers and construct a
    `peers_list_subscription_t` atomically, without worrying about whether there
    was a connection or disconnection in between. The approved way to do that is
    to construct a `peers_list_freeze_t` and not block while it exists. The
    latter is what actually prevents race conditions; connection and
    disconnection events cannot be processed while something else is holding the
    CPU. The purpose of the `peers_list_freeze_t` is to trip an assertion if you
    screws up by blocking at the wrong time. If a connection or disconnection
    event would be delivered while the `peers_list_freeze_t` exists, it will
    trip an assertion. */
    class peers_list_freeze_t {
    public:
        explicit peers_list_freeze_t(connectivity_service_t *);
        void assert_is_holding(connectivity_service_t *);
    private:
        rwi_lock_assertion_t::read_acq_t acq;
    };

    /* `peers_list_subscription_t` will call the given functions when a peer
    connects or disconnects. */
    class peers_list_subscription_t {
    public:
        explicit peers_list_subscription_t(peers_list_callback_t *connect_disconnect_cb);
        void reset();
        void reset(connectivity_service_t *, peers_list_freeze_t *proof);
    private:
        publisher_t<peers_list_callback_t *>::subscription_t subs;

        DISABLE_COPYING(peers_list_subscription_t);
    };

    /* `get_me()` returns the `peer_id_t` for this cluster node.
    `get_peers_list()` returns all the currently-accessible peers in the cluster
    and their addresses, including us. */
    virtual peer_id_t get_me() = 0;
    virtual std::set<peer_id_t> get_peers_list() = 0;

    virtual bool get_peer_connected(peer_id_t peer) {
        return get_peers_list().count(peer) == 1;
    }

    /* `get_connection_session_id()` returns a UUID for the given peer that
    changes every time the peer disconnects and reconnects. This information
    could be reconstructed by watching connection and disconnection events, but
    it would be hard to reconstruct it consistently across multiple threads. The
    connectivity layer can do it trivially. */
    virtual uuid_t get_connection_session_id(peer_id_t) = 0;

protected:
    virtual ~connectivity_service_t() { }

private:
    virtual rwi_lock_assertion_t *get_peers_list_lock() = 0;
    virtual publisher_t<peers_list_callback_t *> *get_peers_list_publisher() = 0;
};

class disconnect_watcher_t : public signal_t, private peers_list_callback_t {
public:
    disconnect_watcher_t(connectivity_service_t *, peer_id_t);
private:
    void on_disconnect(peer_id_t p);
    void on_connect(UNUSED peer_id_t p) { }
    connectivity_service_t::peers_list_subscription_t subs;
    peer_id_t peer;
};

#endif /* RPC_CONNECTIVITY_CONNECTIVITY_HPP_ */
