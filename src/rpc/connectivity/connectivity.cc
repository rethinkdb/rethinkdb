#include "rpc/connectivity/connectivity.hpp"
#include <sstream>
#include <ios>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include "concurrency/drain_semaphore.hpp"

/* event_watcher_t */

event_watcher_t::event_watcher_t(connectivity_cluster_t *parent) :
    cluster(parent)
{
    cluster->assert_thread();
    mutex_acquisition_t acq(&cluster->watchers_mutex);
    cluster->watchers.push_back(this);
}

event_watcher_t::~event_watcher_t() {
    cluster->assert_thread();
    mutex_acquisition_t acq(&cluster->watchers_mutex);
    cluster->watchers.remove(this);
}

/* connect_watcher_t */

connect_watcher_t::connect_watcher_t(connectivity_cluster_t *parent, peer_id_t peer) :
    event_watcher_t(parent), peer(peer)
{
    rassert(!peer.is_nil());
    std::map<peer_id_t, peer_address_t> everybody = parent->get_everybody();
    if (everybody.find(peer) != everybody.end()) {
        pulse();
    }
}

void connect_watcher_t::on_connect(peer_id_t p) {
    if (peer == p && !is_pulsed()) {
        pulse();
    }
}

void connect_watcher_t::on_disconnect(peer_id_t) {
    // Ignore this event
}

/* disconnect_watcher_t */

disconnect_watcher_t::disconnect_watcher_t(connectivity_cluster_t *parent, peer_id_t peer) :
    event_watcher_t(parent), peer(peer)
{
    rassert(!peer.is_nil());
    std::map<peer_id_t, peer_address_t> everybody = parent->get_everybody();
    if (everybody.find(peer) == everybody.end()) {
        pulse();
    }
}

void disconnect_watcher_t::on_connect(peer_id_t) {
    // Ignore this event
}

void disconnect_watcher_t::on_disconnect(peer_id_t p) {
    if (peer == p && !is_pulsed()) {
        pulse();
    }
}

/* connectivity_cluster_t */

connectivity_cluster_t::connectivity_cluster_t(int port) :
    me(peer_id_t(generate_uuid()))
{
    /* Put ourselves in the routing table */
    routing_table[me] = peer_address_t(ip_address_t::us(), port);

    /* Start listening for peers */
    listener.reset(
        new streamed_tcp_listener_t(
            port,
            boost::bind(&connectivity_cluster_t::on_new_connection, this, _1)
        ));
}

connectivity_cluster_t::~connectivity_cluster_t() {
    listener.reset();
    shutdown_cond.pulse();
    shutdown_semaphore.drain();
}

void connectivity_cluster_t::join(peer_address_t address) {
    assert_thread();
    drain_semaphore_t::lock_t drain_semaphore_lock(&shutdown_semaphore);
    coro_t::spawn_now(boost::bind(
        &connectivity_cluster_t::join_blocking,
        this,
        address,
        /* We don't know what `peer_id_t` the peer has until we connect to it */
        boost::none,
        drain_semaphore_lock
        ));
}

peer_id_t connectivity_cluster_t::get_me() {
    assert_thread();
    return me;
}

std::map<peer_id_t, peer_address_t> connectivity_cluster_t::get_everybody() {
    assert_thread();
    /* We can't just return `routing_table` because `routing_table` includes
    some partially-connected peers, so if we just returned `routing_table` then
    some peers would appear in the output from `get_everybody()` before the
    `on_connect()` event was sent out or after the `on_disconnect()` event was
    sent out. */
    std::map<peer_id_t, peer_address_t> peers;
    peers[me] = routing_table[me];
    for (std::map<peer_id_t, connection_t*>::iterator it = connections.begin();
            it != connections.end(); it++) {
        peers[(*it).first] = routing_table[(*it).first];
    }
    return peers;
}

void connectivity_cluster_t::send_message(peer_id_t dest, boost::function<void(std::ostream&)> writer) {
    rassert(!dest.is_nil());

    /* We currently write the message to a `stringstream`, then serialize that
    as a string. It's horribly inefficient, of course. */

    std::stringstream buffer(std::ios_base::out|std::stringstream::binary);
    writer(buffer);

    if (dest == me) {
        std::stringstream buffer2(buffer.str(), std::stringstream::in|std::stringstream::binary);

        /* Spawn `on_message()` directly in a new coroutine */
        cond_t pulse_when_done_reading;
        coro_t::spawn_now(boost::bind(
            &connectivity_cluster_t::on_message,
            this,
            me,
            boost::ref(buffer2),
            boost::function<void()>(boost::bind(&cond_t::pulse, &pulse_when_done_reading))
            ));
        pulse_when_done_reading.wait();

    } else {
        on_thread_t thread_switcher(home_thread());

        std::map<peer_id_t, connection_t*>::iterator it = connections.find(dest);
        if (it == connections.end()) {
            /* We don't currently have access to this peer. Our policy is to not
            notify the sender when a message cannot be transmitted (since this
            is not always possible). So just return. */
            return;
        }
        connection_t *dest_conn = (*it).second;

        /* Acquire the send-mutex so we don't collide with other things trying
        to send on the same connection. */
        mutex_acquisition_t acq(&dest_conn->send_mutex);

        try {
            boost::archive::text_oarchive sender(dest_conn->conn->get_ostream());
            std::string buffer_str = buffer.str();
            sender << buffer_str;
        } catch (boost::archive::archive_exception) {
            /* Close the other half of the connection to make sure that
            `connectivity_cluster_t::handle()` notices that something is up */
            if (dest_conn->conn->is_read_open()) {
                dest_conn->conn->shutdown_read();
            }
        }
    }
}

void connectivity_cluster_t::on_new_connection(boost::scoped_ptr<streamed_tcp_conn_t> &conn) {
    drain_semaphore_t::lock_t drain_semaphore_lock(&shutdown_semaphore);
    handle(conn.get(), boost::none, boost::none);
}

/* `connectivity_cluster_t::join_blocking()` is spawned in a new coroutine by
`connectivity_cluster_t::join()`. It's also run by `connectivity_cluster_t::handle()` when we hear about
a new peer from a peer we are connected to. */

void connectivity_cluster_t::join_blocking(
        peer_address_t address,
        boost::optional<peer_id_t> expected_id,
        UNUSED drain_semaphore_t::lock_t lock) {
    try {
        streamed_tcp_conn_t conn(address.ip, address.port);
        handle(&conn, expected_id, boost::optional<peer_address_t>(address));
    } catch (tcp_conn_t::connect_failed_exc_t) {
        /* Ignore */
    }
}

/* `connectivity_cluster_t::handle` is responsible for the entire lifetime of an
intra-cluster TCP connection. It handles the handshake, exchanging node maps,
sending out the connect-notification, receiving messages from the peer until it
disconnects or we are shut down, and sending out the disconnect-notification. */

void connectivity_cluster_t::handle(
        /* `conn` should remain valid until `connectivity_cluster_t::handle()` returns.
        `connectivity_cluster_t::handle()` does not take ownership of `conn`. */
        streamed_tcp_conn_t *conn,
        boost::optional<peer_id_t> expected_id,
        boost::optional<peer_address_t> expected_address) {

    /* Make sure that if we're ordered to shut down, any pending read or write
    gets interrupted. */
    struct connection_closer_t : public signal_t::waiter_t {
        connection_closer_t(signal_t *s, streamed_tcp_conn_t *c) :
            signal(s), conn_to_close(c)
        {
            signal->add_waiter(this);
        }
        ~connection_closer_t() {
            if (!signal->is_pulsed()) {
                signal->remove_waiter(this);
            }
        }
        signal_t *signal;
        streamed_tcp_conn_t *conn_to_close;
        void on_signal_pulsed() {
            if (conn_to_close->is_read_open()) {
                conn_to_close->shutdown_read();
            }
            if (conn_to_close->is_write_open()) {
                conn_to_close->shutdown_write();
            }
        }
    } connection_closer(&shutdown_cond, conn);

    /* Each side sends their own ID and address, then receives the other side's.
    */

    try {
        boost::archive::text_oarchive sender(conn->get_ostream());
        sender << me;
        sender << routing_table[me];
    } catch (boost::archive::archive_exception) {
        /* We expect that sending can fail due to a network problem. If that
        happens, just ignore it. If sending fails for some other reason, then
        the programmer should learn about it, so we rethrow the exception. */
        if (!conn->is_write_open()) return;
        else throw;
    }

    peer_id_t other_id;
    peer_address_t other_address;
    try {
        boost::archive::text_iarchive receiver(conn->get_istream());
        receiver >> other_id;
        receiver >> other_address;
    } catch (boost::archive::archive_exception) {
        if (!conn->is_read_open()) return;
        else throw;
    }

    /* Sanity checks */
    if (other_id == me) {
        crash("Help, I'm being impersonated!");
    }
    if (other_id.is_nil()) {
        crash("Peer is nil");
    }
    if (expected_id && other_id != *expected_id) {
        crash("Inconsistent routing information: wrong ID");
    }
    if (expected_address && other_address != *expected_address) {
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

    std::map<peer_id_t, peer_address_t> other_routing_table;

    if (we_are_leader) {

        std::map<peer_id_t, peer_address_t> routing_table_to_send;

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
            boost::archive::text_oarchive sender(conn->get_ostream());
            sender << routing_table_to_send;
        } catch (boost::archive::archive_exception) {
            routing_table.erase(other_id);
            if (!conn->is_write_open()) return;
            else throw;
        }

        /* Receive the follower's routing table */
        try {
            boost::archive::text_iarchive receiver(conn->get_istream());
            receiver >> other_routing_table;
        } catch (boost::archive::archive_exception) {
            routing_table.erase(other_id);
            if (!conn->is_read_open()) return;
            else throw;
        }

    } else {

        /* Receive the leader's routing table. (If our connection has lost a
        conflict, then the leader will close the connection instead of sending
        the routing table. */
        try {
            boost::archive::text_iarchive receiver(conn->get_istream());
            receiver >> other_routing_table;
        } catch (boost::archive::archive_exception) {
            if (!conn->is_read_open()) return;
            else throw;
        }

        std::map<peer_id_t, peer_address_t> routing_table_to_send;

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
            boost::archive::text_oarchive sender(conn->get_ostream());
            sender << routing_table_to_send;
        } catch (boost::archive::archive_exception) {
            if (!conn->is_write_open()) return;
            else throw;
        }
    }

    /* For each peer that our new friend told us about that we don't already
    know about, start a new connection. If the cluster is shutting down, skip
    this step. */
    if (!shutdown_cond.is_pulsed()) {
        /* Acquire the drain semaphore so we can pass a
        `drain_semaphore_t::lock_t` to the coroutine we spawn. That way, the
        `connectivity_cluster_t` won't shut down while there coroutine is still
        active. */
        drain_semaphore_t::lock_t drain_semaphore_lock(&shutdown_semaphore);

        for (std::map<peer_id_t, peer_address_t>::iterator it = other_routing_table.begin();
             it != other_routing_table.end(); it++) {
            if (routing_table.find((*it).first) == routing_table.end()) {
                /* `(*it).first` is the ID of a peer that our peer is connected
                to, but we aren't connected to. */
                coro_t::spawn_now(boost::bind(
                    &connectivity_cluster_t::join_blocking, this,
                    (*it).second,
                    boost::optional<peer_id_t>((*it).first),
                    drain_semaphore_lock));
            }
        }
    }

    {
        /* `connection_t` is the public interface of this coroutine. We put a
        pointer to `conn_structure` into `connections`, and clients use that to
        find us for the purpose of sending messages. */
        connection_t conn_structure;
        conn_structure.conn = conn;

        /* Put ourselves in the connection-map and notify event-watchers that
        the connection is up */
        {
            mutex_acquisition_t acq(&watchers_mutex);

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
                {
                    boost::archive::text_iarchive receiver(conn->get_istream());
                    receiver >> message;
                }

                /* In case anyone is tempted to refactor this so it doesn't wait
                on `pulse_when_done_reading`, refrain. It may look silly now,
                but later we will want to read directly off the socket for
                better performance, and then we will need this code. */
                std::stringstream stream(message, std::stringstream::in|std::stringstream::binary);
                cond_t pulse_when_done_reading;
                coro_t::spawn_now(boost::bind(
                    &connectivity_cluster_t::on_message,
                    this,
                    other_id,
                    boost::ref(stream),
                    boost::function<void()>(boost::bind(&cond_t::pulse, &pulse_when_done_reading))
                    ));
                pulse_when_done_reading.wait();
            }
        } catch (boost::archive::archive_exception) {
            /* The exception broke us out of the loop, and that's what we
            wanted. */
            if (conn->is_read_open()) throw;
        }

        /* Remove us from the connection map. */
        {
            mutex_acquisition_t acq(&watchers_mutex);

            /* This is kind of tricky: We want new calls to `send_message()` to
            see that we are gone, but we don't want to cause segfaults by
            destroying the `connection_t` while something is still using it. The
            solution is to remove our entry in the connections map (so nothing
            new acquires the `send_mutex`) and then acquire the `send_mutex`
            ourself (so anything that already was in line for the `send_mutex`
            gets a chance to finish). */
            rassert(connections[other_id] == &conn_structure);
            connections.erase(other_id);
            co_lock_mutex(&conn_structure.send_mutex);

            /* `event_watcher_t`s shouldn't block. */
            ASSERT_FINITE_CORO_WAITING;

            for (event_watcher_t *w = watchers.head(); w; w = watchers.next(w)) {
                w->on_disconnect(other_id);
            }
        }
    }

    /* Remove us from the routing map */
    routing_table.erase(other_id);
}
