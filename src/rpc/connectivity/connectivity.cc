#include "rpc/connectivity/connectivity.hpp"

namespace connectivity {

cluster_t::cluster_t(int port) :
    listener(port),
    me(peer_id_t(boost::uuids::random_generator()()))
{
    routing_table[me] = address_t(ip_address_t::me(), port);
}

cluster_t::cluster_t(int port, peer_id_t id_from_last_time) :
    listener(port),
    me(id_from_last_time)
{
    routing_table[me] = address_t(ip_address_t::me(), port);
}

cluster_t::~cluster_t() {
    shutdown_cond.pulse();
    shutdown_semaphore.drain();
}

void cluster_t::join(address_t address) {

    streamed_tcp_conn_t *conn = new streamed_tcp_conn_t(new tcp_conn_t(address.ip, address.port));

    drain_semaphore_t::lock_t drain_semaphore_lock(&shutdown_semaphore);

    coro_t::spawn(boost::bind(
        &cluster_t::handle,
        conn,
        boost::none,
        boost::optional<address_t>(address),
        drain_semaphore_lock
        ));
}

peer_id_t cluster_t::get_me() {
    return me;
}

std::map<peer_id_t, address_t> cluster_t::get_everybody() {
    /* We can't just return `routing_table` because `routing_table` includes
    some partially-connected peers, so if we just returned `routing_table` then
    some peers would appear in the output from `get_everybody()` before the
    `on_connect()` event was sent out or after the `on_disconnect()` event was
    sent out. */
    std::map<peer_id_t, address_t> peers;
    peers[me] = routing_table[me];
    for (std::map<peer_id_t, connection_t*>::iterator it = connections.begin();
            it != connections.end(); it++) {
        peers[(*it).first] = routing_table[(*it).first];
    }
    return peers;
}

/* cluster_t::event_watcher_t */

cluster_t::event_watcher_t::event_watcher_t(cluster_t *parent) :
    cluster(parent)
{
    mutex_acquisition_t acq(&parent->watcher_mutex);
    parent->watchers.push_back(this);
}

cluster_t::event_watcher_t::~event_watcher_t() {
    mutex_acquisition_t acq(&parent->watcher_mutex);
    parent->watchers.remove(this);
}

/* cluster_t::connect_watcher_t */

cluster_t::connect_watcher_t::connect_watcher_t(cluster_t *parent, peer_id_t peer) :
    event_watcher_t(parent), peer(peer) { }

void cluster_t::connect_watcher_t::on_connect(peer_id_t p) {
    if (peer == p && !is_pulsed()) {
        pulse();
    }
}

void cluster_t::connect_watcher_t::on_disconnect(peer_id_t) {
    // Ignore this event
}

/* cluster_t::disconnect_watcher_t */

cluster_t::disconnect_watcher_t::disconnect_watcher_t(cluster_t *parent, peer_id_t peer) :
    event_watcher_t(parent), peer(peer) { }

void cluster_t::disconnect_watcher_t::on_connect(peer_id_t) {
    // Ignore this event
}

void cluster_t::disconnect_watcher_t::on_disconnect(peer_id_t p) {
    if (peer == p && !is_pulsed()) {
        pulse();
    }
}

void cluster_t::send_message(peer_id_t dest, boost::function<void(std::ostream&)> writer) {

    std::stringstream buffer(ios_base::out);
    writer(buffer);

    if (dest == me) {
        std::stringstream buffer2(buffer.str(), ios_base::in);
        on_message(me, buffer2);

    } else {
        connection_t *dest = connections[dest];
        if (!dest) {
            /* We don't currently have access to this peer. Our policy is to not
            notify the sender when a message cannot be transmitted (since this
            is not always possible). So just return. */
            return;
        }

        /* Acquire the send-mutex so we don't collide with other things trying
        to send on the same connection. */
        mutex_acquisition_t acq(&dest->send_mutex);

        try {
            boost::archive::xml_oarchive sender(dest->conn->stream);
            sender << buffer.str();
        } catch (tcp_conn_t::write_closed_exc_t) {
            /* Close the other half of the connection to make sure that
            `cluster_t::handle()` notices that something is up */
            if (dest->conn->conn->is_read_open()) {
                dest->conn->conn->shutdown_read();
            }
        }
    }
}

/* `cluster_t::handle` is responsible for the entire lifetime of an
intra-cluster TCP connection. It handles the handshake, exchanging node maps,
sending out the connect-notification, receiving messages from the peer until it
disconnects or we are shut down, and sending out the disconnect-notification. */

void cluster_t::handle(streamed_tcp_conn_t *c,
        boost::optional<peer_id_t> expected_id,
        boost::optional<address_t> expected_address,
        UNUSED drain_semaphore_t::lock_t drain_semaphore_lock) {

    /* Put the connection in a `boost::scoped_ptr` so that it won't leak if we
    get an exception */
    boost::scoped_ptr<streamed_tcp_conn_t> conn(c);

    /* Make sure that if we're ordered to shut down, any pending read or write
    gets interrupted. */
    struct connection_closer_t : public signal_t::watcher_t {
        streamed_tcp_conn_t *conn_to_close;
        void on_signal_pulsed() {
            if (conn_to_close->conn->is_read_open()) {
                conn_to_close->conn->shutdown_read();
            }
            if (conn_to_close->conn->is_write_open()) {
                conn_to_close->conn->shutdown_write();
            }
        }
    };
    connection_closer_t connection_closer;
    shutdown_cond.add_watcher(&connection_closer);

    /* Use XML as a joke */
    boost::archive::xml_oarchive sender(conn->stream);
    boost::archive::xml_iarchive receiver(conn->stream);

    /* Each side sends their own ID and address, then receives the other side's.
    */

    try {
        sender << me;
        sender << routing_table[me];
    } catch (tcp_conn_t::write_closed_exc_t) {
        return;
    }

    peer_id_t other_id;
    address_t other_address;
    try {
        receiver >> other_id;
        receiver >> other_address;
    } catch (tcp_conn_t::read_closed_exc_t) {
        return;
    }

    /* Sanity checks */
    if (other_id == me) {
        crash("Help, I'm being impersonated!");
    }
    if (expected_id && other_id != *expected_id) {
        crash("Inconsistent routing information: wrong ID");
    }
    if (expected_addr && other_address != *expected_addr) {
        crash("Inconsistent routing information: wrong address");
    }

    /* We pick one side of the connection to be the "leader" and the other side
    to be the "follower". These roles are only relevant in the initial startup
    process. The leader registers the connection locally. If there's a conflict,
    it drops the connection. If not, it sends its routing table to the follower.
    Then the follower registers itself locally. There shouldn't be a conflict
    because any duplicate connection would have been detected by the leader.
    Then the follower sends its routing table to the leader. */
    bool we_are_leader = me < other_id;

    std::map<peer_id_t, address_t> other_routing_table;

    if (we_are_leader) {

        std::map<peer_id_t, address_t> routing_table_to_send;

        /* Critical section: we must check for conflicts and register ourself
        without the interference of any other connections. This ensures that
        any conflicts are resolved consistently. It also ensures that if we get
        two connections from different nodes, one will find out about the other.
        */
        {
            mutex_acquisition_t acq(&new_connection_mutex);

            if (routing_table.find(other_id) != routing_table.end()) {
                /* Conflict! Abort! Terminate the connection unceremoniously;
                the follower will find out. */
                return;
            }

            /* Make a copy of `routing_table` before exiting the critical
            section */
            routing_table_to_send = routing_table;

            /* Register ourselves while in the critical section, so that whoever
            comes next will see us */
            routing_table[other_id] = other_address;
        }

        /* We're good to go! Transmit the routing table to the follower, so it
        knows we're in. */
        try {
            sender << routing_table_to_send;
        } catch (tcp_conn_t::write_failed_exc_t) {
            routing_table.erase(other_id);
            return;
        }

        /* Receive the follower's routing table */
        try {
            receiver >> other_routing_table;
        } catch (tcp_conn_t::read_failed_exc_t) {
            routing_table.erase(other_id);
            return;
        }

    } else {

        /* Receive the leader's routing table. (If our connection has lost a
        conflict, then the leader will close the connection instead of sending
        the routing table. */
        try {
            receiver >> other_routing_table;
        } catch (tcp_conn_t::read_failed_exc_t) {
            return;
        }

        std::map<peer_id_t, address_t> routing_table_to_send;

        /* Register ourselves locally. This is in a critical section so that if
        we get two connections from different nodes at the same time, one will
        find out about the other. */
        {
            mutex_acquisition_t acq(&new_connection_mutex);

            if (routing_table.find(other_id) != routing_table.end()) {
                crash("Why didn't the leader detect this conflict?");
            }

            /* Make a copy of `routing_table` before exiting the critical
            section */
            routing_table_to_send = routing_table;

            /* Register ourselves while in the critical section, so that whoever
            comes next will see us */
            routing_table[other_id] = other_address;
        }

        /* Send our routing table to the leader */
        try {
            sender << routing_table_to_send;
        } catch (tcp_conn_t::write_failed_exc_t) {
            routing_table.erase(other_id);
            return;
        }
    }

    /* For each peer that our new friend told us about that we don't already
    know about, start a new connection */
    for (std::map<peer_id_t, address_t>::iterator it = other_routing_table.begin();
         it != other_routing_table.end(); it++) {
        if (routing_table.find((*it).first) != routing_table.end()) {
            /* Start the process of connecting to the newly-discovered peer */
            coro_t::spawn(boost::bind(
                &cluster_t::handle,
                this,
                new tcp_conn_t((*it).second.host, (*it).second.port),
                boost::optional<peer_id_t>((*it).first),
                boost::optional<address_t>((*it).second),
                drain_semaphore_lock
                ));
        }
    }

    {
        /* `connection_t` is the public interface of this coroutine. We put a
        pointer to `conn_structure` into `connections`, and clients use that to
        find us for the purpose of sending messages. */
        connection_t conn_structure;
        conn_structure.conn = conn.get();

        /* Put ourselves in the connection-map and notify event-watchers that
        the connection is up */
        {
            mutex_acquisition_t acq(&watcher_mutex);

            rassert(connections.find(other_id) == connections.end());
            connections[other_id] = &conn_structure;

            /* `event_watcher_t`s shouldn't block. */
            ASSERT_FINITE_CORO_WAITING;

            for (event_watcher_t *w = watchers.head(); w; w = watchers.next(w)) {
                w->on_connect(other_id);
            }
        }

        /* Main message-handling loop: read messages off the connection until
        it's closed, which may be due to network events, or the other end
        shutting down, or us shutting down. */
        try {
            while (true) {
                /* For now, we use `std::string` for messages on the wire: it's
                just a length and a byte vector. This is obviously slow and we
                should change it when we care about performance. */
                std::string message;
                receiver >> message;
                std::stringstream stream(message, ios_base::in);
                on_message(other_id, stream);
            }
        } catch (tcp_conn_t::read_closed_exc_t) {
            /* The exception broke us out of the loop, and that's what we
            wanted. */
        }

        /* Remove us from the connection map. */
        {
            mutex_acquisition_t acq(&watcher_mutex);

            /* This is kind of tricky: We want new calls to `send_message()` to
            see that we are gone, but we don't want to cause segfaults by
            destroying the `connection_t` while something is still using it. The
            solution is to remove our entry in the connections map (so nothing
            new acquires the `send_mutex`) and then acquire the `send_mutex`
            ourself (so anything that already was in line for the `send_mutex`
            gets a chance to finish). */
            rassert(connections[other_id] == &conn_structure);
            connections.erase(other_id);
            conn_structure.send_mutex.lock();

            /* `event_watcher_t`s shouldn't block. */
            ASSERT_FINITE_CORO_WAITING;

            for (event_watcher_t *w = watchers.head(); w; w = watchers.next(w)) {
                w->on_disconnect(other_id);
            }
        }
    }

    /* Remove us from the routing map */
    routing_table.erase(other_id);

    /* Get rid of the `connection_closer_t` */
    if (!shutdown_cond.is_pulsed()) {
        shutdown_cond.remove_watcher(&connection_closer);
    }
}

}   /* namespace connectivity */
