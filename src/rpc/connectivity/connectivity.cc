#include "rpc/connectivity/connectivity.hpp"

#include <sstream>
#include <ios>

#include "utils.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>

#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/drain_semaphore.hpp"
#include "concurrency/pmap.hpp"

/* event_watcher_t */

event_watcher_t::event_watcher_t(connectivity_cluster_t *parent) :
    cluster(parent)
{
    cluster->assert_thread();
    mutex_acquisition_t acq(&cluster->watchers_mutexes_by_thread[get_thread_id()]);
    cluster->watchers_by_thread[get_thread_id()].push_back(this);
}

event_watcher_t::~event_watcher_t() {
    cluster->assert_thread();
    mutex_acquisition_t acq(&cluster->watchers_mutexes_by_thread[get_thread_id()]);
    cluster->watchers_by_thread[get_thread_id()].remove(this);
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

void connectivity_cluster_t::join(peer_address_t address) {
    assert_thread();
    coro_t::spawn_now(boost::bind(
        &connectivity_cluster_t::join_blocking,
        this,
        address,
        /* We don't know what `peer_id_t` the peer has until we connect to it */
        boost::none,
        auto_drainer_t::lock_t(&drainer)
        ));
}

peer_id_t connectivity_cluster_t::get_me() {
    assert_thread();
    return me;
}

std::map<peer_id_t, peer_address_t> connectivity_cluster_t::get_everybody() {
    // TODO THREAD we can remove this assert thread when we can call
    // event watchers with cross thread messages.  But that's not
    // necessarily going to happen.
    assert_thread();

    /* We can't just return `routing_table` because `routing_table` includes
    some partially-connected peers, so if we just returned `routing_table` then
    some peers would appear in the output from `get_everybody()` before the
    `on_connect()` event was sent out or after the `on_disconnect()` event was
    sent out. */

    // TODO THREAD: Consider whether it is correct to just use the rpc
    // listener's connections map.  See who uses get_everybody.
    const std::map<peer_id_t, connection_t *>& connections = connection_maps_by_thread[get_thread_id()];

    std::map<peer_id_t, peer_address_t> peers;
    peers[me] = routing_table[me];
    for (std::map<peer_id_t, connection_t*>::const_iterator it = connections.begin();
            it != connections.end(); it++) {
        peers[it->first] = it->second->address;
    }
    return peers;
}

connectivity_cluster_t::connectivity_cluster_t(int port)
    : me(peer_id_t(generate_uuid())),
      connection_maps_by_thread(get_num_threads()),
      watchers_by_thread(new intrusive_list_t<event_watcher_t>[get_num_threads()]),
      watchers_mutexes_by_thread(new mutex_t[get_num_threads()])
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

    /* The destructor for `drainer` takes care of making sure that all the
    coroutines we spawned shut down. */
}

void connectivity_cluster_t::send_message(peer_id_t dest, boost::function<void(std::ostream&)> writer) {
    // We could be on _any_ thread.

    rassert(!dest.is_nil());

    /* We currently write the message to a `stringstream`, then
       serialize that as a string. It's horribly inefficient, of course. */
    // TODO: If we don't do it this way, we (or the caller) will need
    // to worry about having the writer run on the connection thread.

    std::stringstream buffer(std::ios_base::out | std::stringstream::binary);
    writer(buffer);

#ifdef CLUSTER_MESSAGE_DEBUGGING
    std::cerr << "from " << me << " to " << dest << std::endl;
    print_hd(buffer.str().data(), 0, buffer.str().size());
#endif

    if (dest == me) {

        // The destination "connection"'s home thread is the rpc
        // listener thread.

        // TODO THREAD do this operation asynchronously, if we can.
        // (Maybe not.  Maybe the caller can be asynchronous, if it
        // cares.)
        on_thread_t threader(home_thread());

        // Of course we're on the connection thread, since dest == me,
        // but we're just trying to make a point here.
        assert_connection_thread(dest);

        std::stringstream buffer2(buffer.str(), std::stringstream::in | std::stringstream::binary);

        /* Spawn `on_message()` directly in a new coroutine */
        debugf("spawning weird on_message Now.\n");
        cond_t pulse_when_done_reading;
        coro_t::spawn_now(boost::bind(
            &connectivity_cluster_t::on_message,
            this,
            me,
            boost::ref(buffer2),
            boost::function<void()>(boost::bind(&cond_t::pulse, &pulse_when_done_reading))
            ));
        pulse_when_done_reading.wait();

        // TODO THREAD why do we pulse_when_done_reading.wait()?  is
        // it for object lifetime or is it so that we're done reading
        // when this function returns?

    } else {
        const std::map<peer_id_t, connection_t *>& connections = connection_maps_by_thread[get_thread_id()];

        std::map<peer_id_t, connection_t *>::const_iterator it = connections.find(dest);
        if (it == connections.end()) {
            /* We don't currently have access to this peer. Our policy is to not
            notify the sender when a message cannot be transmitted (since this
            is not always possible). So just return. */
            return;
        }
        connection_t *dest_conn = it->second;

        // TODO THREAD do this operation asynchronously.  (Again,
        // maybe not.  Maybe the caller can call this function
        // asynchronously, if he cares to.)
        on_thread_t threader(dest_conn->conn->home_thread());

        // Kind of redundant, again, since we explicitly visited
        // connections.find(dest)->second->conn->home_thread().
        assert_connection_thread(dest);

        /* Acquire the send-mutex so we don't collide with other things trying
        to send on the same connection. */
        mutex_acquisition_t acq(&dest_conn->send_mutex);

        try {
            boost::archive::binary_oarchive sender(dest_conn->conn->get_ostream());
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

#ifndef NDEBUG
void connectivity_cluster_t::assert_connection_thread(peer_id_t peer) const {
    if (peer != me) {
        const std::map<peer_id_t, connection_t *>& map = connection_maps_by_thread[get_thread_id()];

        std::map<peer_id_t, connection_t *>::const_iterator it = map.find(peer);
        rassert(it != map.end());
        rassert(it->second->conn->home_thread() == get_thread_id());
    } else {
        assert_thread();
    }
}
#endif

void connectivity_cluster_t::on_new_connection(boost::scoped_ptr<streamed_tcp_conn_t> &conn) {
    assert_thread();

    handle(conn.get(), boost::none, boost::none,
        auto_drainer_t::lock_t(&drainer));
}

/* `connectivity_cluster_t::join_blocking()` is spawned in a new coroutine by
`connectivity_cluster_t::join()`. It's also run by
`connectivity_cluster_t::handle()` when we hear about a new peer from a peer we
are connected to. */

void connectivity_cluster_t::join_blocking(
        peer_address_t address,
        boost::optional<peer_id_t> expected_id,
        auto_drainer_t::lock_t drainer_lock) {

    assert_thread();

    try {
        streamed_tcp_conn_t conn(address.ip, address.port);
        handle(&conn, expected_id, boost::optional<peer_address_t>(address), drainer_lock);
    } catch (tcp_conn_t::connect_failed_exc_t) {
        /* Ignore */
    }
}

/* `connectivity_cluster_t::handle` is responsible for the entire lifetime of an
intra-cluster TCP connection. It handles the handshake, exchanging node maps,
sending out the connect-notification, receiving messages from the peer until it
disconnects or we are shut down, and sending out the disconnect-notification. */

static void close_conn(streamed_tcp_conn_t *c) {
    if (c->is_read_open()) {
        c->shutdown_read();
    }
    if (c->is_write_open()) {
        c->shutdown_write();
    }
}


void connectivity_cluster_t::handle(
        /* `conn` should remain valid until `connectivity_cluster_t::handle()`
        returns. `connectivity_cluster_t::handle()` does not take ownership of
        `conn`. */
        streamed_tcp_conn_t *conn,
        boost::optional<peer_id_t> expected_id,
        boost::optional<peer_address_t> expected_address,
        auto_drainer_t::lock_t drainer_lock) {
    assert_thread();

    /* Each side sends their own ID and address, then receives the other side's.
    */

    try {
        boost::archive::binary_oarchive sender(conn->get_ostream());
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
        boost::archive::binary_iarchive receiver(conn->get_istream());
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

    // Just saying that we're still on the rpc listener thread.
    assert_thread();

    /* The trickiest case is when there are two or more parallel connections
    that are trying to be established between the same two machines. We can get
    this when e.g. machine A and machine B try to connect to each other at the
    same time. It's important that exactly one of the connections actually gets
    established. When there are multiple connections trying to be established,
    this is referred to as a "conflict". */

    boost::scoped_ptr<map_insertion_sentry_t<peer_id_t, peer_address_t> >
        routing_table_entry_sentry;

    /* We pick one side of the connection to be the "leader" and the other side
    to be the "follower". These roles are only relevant in the initial startup
    process. The leader registers the connection locally. If there's a conflict,
    it drops the connection. If not, it sends its routing table to the follower.
    Then the follower registers itself locally. There shouldn't be a conflict
    because any duplicate connection would have been detected by the leader.
    Then the follower sends its routing table to the leader. */
    bool we_are_leader = me < other_id;

    // Just saying: Still on rpc listener thread, for
    // sending/receiving routing table
    assert_thread();
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
            routing_table_entry_sentry.reset(
                new map_insertion_sentry_t<peer_id_t, peer_address_t>(
                    &routing_table, other_id, other_address
                    ));
        }

        /* We're good to go! Transmit the routing table to the follower, so it
        knows we're in. */
        try {
            boost::archive::binary_oarchive sender(conn->get_ostream());
            sender << routing_table_to_send;
        } catch (boost::archive::archive_exception) {
            if (!conn->is_write_open()) {
                return;
            } else {
                throw;
            }
        }

        /* Receive the follower's routing table */
        try {
            boost::archive::binary_iarchive receiver(conn->get_istream());
            receiver >> other_routing_table;
        } catch (boost::archive::archive_exception) {
            if (!conn->is_read_open()) {
                return;
            } else {
                throw;
            }
        }

    } else {

        /* Receive the leader's routing table. (If our connection has lost a
        conflict, then the leader will close the connection instead of sending
        the routing table. */
        try {
            boost::archive::binary_iarchive receiver(conn->get_istream());
            receiver >> other_routing_table;
        } catch (boost::archive::archive_exception) {
            if (!conn->is_read_open()) {
                return;
            } else {
                throw;
            }
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
            routing_table_entry_sentry.reset(
                new map_insertion_sentry_t<peer_id_t, peer_address_t>(
                    &routing_table, other_id, other_address
                    ));
        }

        /* Send our routing table to the leader */
        try {
            boost::archive::binary_oarchive sender(conn->get_ostream());
            sender << routing_table_to_send;
        } catch (boost::archive::archive_exception) {
            if (!conn->is_write_open()) {
                return;
            } else {
                throw;
            }
        }
    }

    // Just saying: We haven't left the RPC listener thread.
    assert_thread();

    /* For each peer that our new friend told us about that we don't already
    know about, start a new connection. If the cluster is shutting down, skip
    this step. */
    if (!drainer_lock.get_drain_signal()->is_pulsed()) {
        for (std::map<peer_id_t, peer_address_t>::iterator it = other_routing_table.begin();
             it != other_routing_table.end(); it++) {
            if (routing_table.find(it->first) == routing_table.end()) {
                /* `it->first` is the ID of a peer that our peer is connected
                to, but we aren't connected to. */
                coro_t::spawn_now(boost::bind(
                    &connectivity_cluster_t::join_blocking, this,
                    it->second,
                    boost::optional<peer_id_t>(it->first),
                    drainer_lock));
            }
        }
    }

    // TODO THREAD pick a better way to pick a better thread
    int chosen_thread = rng.randint(get_num_threads());

    cross_thread_signal_t connection_thread_drain_signal(drainer_lock.get_drain_signal(), chosen_thread);

    rethread_streamed_tcp_conn_t unregister_conn(conn, INVALID_THREAD);
    on_thread_t conn_threader(chosen_thread);
    rethread_streamed_tcp_conn_t reregister_conn(conn, get_thread_id());

    // Make sure that if we're ordered to shut down, any pending read
    // or write gets interrupted.
    signal_t::subscription_t conn_closer(boost::bind(&close_conn, conn));
    if (connection_thread_drain_signal.is_pulsed()) {
        close_conn(conn);
    } else {
        conn_closer.resubscribe(&connection_thread_drain_signal);
    }

    {
        /* `connection_t` is the public interface of this coroutine. We put a
        pointer to `conn_structure` into `connections`, and clients use that to
        find us for the purpose of sending messages. */
        connection_t conn_structure;
        conn_structure.conn = conn;
        conn_structure.address = other_address;

        /* Put ourselves in the connection-map and notify event-watchers that
        the connection is up */
        {
#ifndef NDEBUG
            std::map<peer_id_t, connection_t *>& connections = connection_maps_by_thread[get_thread_id()];
            rassert(connections.find(other_id) == connections.end());
#endif

            pmap(get_num_threads(), boost::bind(&connectivity_cluster_t::set_a_connection_entry, this, _1, other_id, &conn_structure));

            pmap(get_num_threads(), boost::bind(&connectivity_cluster_t::ping_connection_watchers, this, _1, other_id));
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
                    assert(get_thread_id() == chosen_thread);
                    boost::archive::binary_iarchive receiver(conn->get_istream());
                    receiver >> message;
                }

                /* In case anyone is tempted to refactor this so it doesn't wait
                on `pulse_when_done_reading`, refrain. It may look silly now,
                but later we will want to read directly off the socket for
                better performance, and then we will need this code. */
                std::stringstream stream(message, std::stringstream::in | std::stringstream::binary);
                cond_t pulse_when_done_reading;
                debugf("Spawning on_message right now.\n");
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

            // TODO THREAD: We can't just throw, we need to cleanly
            // remove ourselves from the connection map, no?

            if (conn->is_read_open()) {
                throw;
            }
        }

        /* Remove us from the connection map. */
        {
            /* This is kind of tricky: We want new calls to `send_message()` to
            see that we are gone, but we don't want to cause segfaults by
            destroying the `connection_t` while something is still using it. The
            solution is to remove our entry in the connections map (so nothing
            new acquires the `send_mutex`) and then acquire the `send_mutex`
            ourself (so anything that already was in line for the `send_mutex`
            gets a chance to finish). */

            pmap(get_num_threads(), boost::bind(&connectivity_cluster_t::erase_a_connection_entry, this, _1, other_id));

            co_lock_mutex(&conn_structure.send_mutex);

            // TODO THREAD definitely ask tim: we switched the order
            // of acquiring the send_mutex versus acquiring the
            // watchers_mutex.  Supposedly the watchers mutex is
            // worried about the watchers array getting modified as we
            // iterate over it, so I (Sam) don't see why it wouldn't have
            // been superclose to the watchers array.

            // TODO THREAD: We need to switch threads to access the
            // watchers array and watchers_mutex.  Do we really want
            // to do that here?  Or should we let the above
            // on_thread_t (to the connection thread) go out of scope
            // and do it then?  (Similarly, could we have added
            // ourselves to watchers before letting the on_thread_t
            // get into scope?)

            pmap(get_num_threads(), boost::bind(&connectivity_cluster_t::ping_disconnection_watchers, this, _1, other_id));
        }
    }
}

void connectivity_cluster_t::set_a_connection_entry(int target_thread, peer_id_t other_id, connection_t *connection) {
    on_thread_t switcher(target_thread);
    connection_maps_by_thread[target_thread][other_id] = connection;
}

void connectivity_cluster_t::erase_a_connection_entry(int target_thread, peer_id_t other_id) {
    on_thread_t switcher(target_thread);
    connection_maps_by_thread[target_thread].erase(other_id);
}

void connectivity_cluster_t::ping_connection_watchers(int target_thread, peer_id_t other_id) {
    on_thread_t th(target_thread);

    mutex_acquisition_t acq(&watchers_mutexes_by_thread[target_thread]);

    ASSERT_FINITE_CORO_WAITING;

    for (event_watcher_t *w = watchers_by_thread[target_thread].head(); w; w = watchers_by_thread[target_thread].next(w)) {
        w->on_connect(other_id);
    }
}

void connectivity_cluster_t::ping_disconnection_watchers(int target_thread, peer_id_t other_id) {
    on_thread_t th(target_thread);

    mutex_acquisition_t acq(&watchers_mutexes_by_thread[target_thread]);

    ASSERT_FINITE_CORO_WAITING;

    for (event_watcher_t *w = watchers_by_thread[target_thread].head(); w; w = watchers_by_thread[target_thread].next(w)) {
        w->on_disconnect(other_id);
    }
}
