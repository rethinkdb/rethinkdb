#ifndef __RPC_CONNECTIVITY_CONNECTIVITY_HPP__
#define __RPC_CONNECTIVITY_CONNECTIVITY_HPP__

#include <set>

#include "utils.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_serialize.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/utility.hpp>

#include "arch/address.hpp"
#include "concurrency/mutex.hpp"
#include "concurrency/signal.hpp"

class peer_address_t {
public:
    peer_address_t(ip_address_t i, int p) : ip(i), port(p) { }
    peer_address_t() : ip(), port(0) { } // For deserialization
    ip_address_t ip;
    int port;

    bool operator==(const peer_address_t &a) const {
        return ip == a.ip && port == a.port;
    }
    bool operator!=(const peer_address_t &a) const {
        return ip != a.ip || port != a.port;
    }

private:
    friend class ::boost::serialization::access;
    template<class Archive> void serialize(Archive & ar, UNUSED const unsigned int version) {
        ar & ip;
        ar & port;
    }
};

/* `peer_id_t` is a wrapper around a `boost::uuids::uuid`. Each newly
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

    peer_id_t() : uuid(boost::uuids::nil_uuid()) { }

    bool is_nil() const {
        return uuid.is_nil();
    }

private:
    friend class connectivity_cluster_t;
    friend std::ostream &operator<<(std::ostream &, peer_id_t);

    boost::uuids::uuid uuid;
    explicit peer_id_t(boost::uuids::uuid u) : uuid(u) { }

    friend class boost::serialization::access;
    template<class Archive> void serialize(Archive & ar, UNUSED const unsigned int version) {
        ar & uuid;
    }
};

inline std::ostream &operator<<(std::ostream &stream, peer_id_t id) {
    return stream << id.uuid;
}

/* A `connectivity_t` is an object that keeps track of peers that are connected
to us. It's an abstract class because there may be multiple types of
connected-ness between us and other peers. For example, we may be in contact
with another peer but have not received their directory yet, in which case
the `(connectivity_t *)&the_connectivity_cluster` will say that we are connected
but the `(connectivity_t *)&the_directory` will say that we are not. */

class connectivity_t {
public:
    /* While a `peers_list_freeze_t` exists, no connect or disconnect events
    will be delivered. This is so that you can check the status of a peer or
    peers and construct a `peers_list_subscription_t` atomically, without
    worrying about whether there was a connection or disconnection in between.
    Don't block while holding a `peers_list_freeze_t`. */
    class peers_list_freeze_t {
    public:
        peers_list_freeze_t(connectivity_t *);
        void assert_is_holding(connectivity_t *);
    private:
        mutex_assertion_t::acq_t acq;
    };

    /* `peers_list_subscription_t` will call the given functions when a peer
    connects or disconnects. */
    class peers_list_subscription_t {
    public:
        peers_list_subscription_t(
                const boost::function<void(peer_id_t)> &on_connect,
                const boost::function<void(peer_id_t)> &on_disconnect);
        peers_list_subscription_t(
                const boost::function<void(peer_id_t)> &on_connect,
                const boost::function<void(peer_id_t)> &on_disconnect,
                connectivity_t *, peers_list_freeze_t *proof);
        void reset();
        void reset(connectivity_t *, peers_list_freeze_t *proof);
    private:
        publisher_t<std::pair<
                boost::function<void(peer_id_t)>,
                boost::function<void(peer_id_t)>
                > >::subscription_t subs;
    };

    /* `get_me()` returns the `peer_id_t` for this cluster node.
    `get_peers_list()` returns all the currently-accessible peers in the cluster
    and their addresses, including us. */
    virtual peer_id_t get_me() = 0;
    virtual std::set<peer_id_t> get_peers_list() = 0;

    virtual bool get_peer_connected(peer_id_t peer) {
        return get_peers_list().count(peer) == 1;
    }

protected:
    virtual ~connectivity_t() { }

private:
    virtual mutex_assertion_t *get_peers_list_lock() = 0;
    virtual publisher_t<std::pair<
            boost::function<void(peer_id_t)>,
            boost::function<void(peer_id_t)>
            > > *get_peers_list_publisher() = 0;
};

class disconnect_watcher_t : public signal_t {
public:
    disconnect_watcher_t(connectivity_t *, peer_id_t);
private:
    void on_disconnect(peer_id_t);
    connectivity_t::peers_list_subscription_t subs;
    peer_id_t peer;
};

#endif /* __RPC_CONNECTIVITY_CONNECTIVITY_HPP__ */
