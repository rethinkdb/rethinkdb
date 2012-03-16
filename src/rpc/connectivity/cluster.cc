#include "rpc/connectivity/cluster.hpp"

#include <sstream>
#include <ios>

#include "utils.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>

#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/pmap.hpp"
#include "containers/uuid.hpp"
#include "do_on_thread.hpp"

connectivity_cluster_t::run_t::run_t(connectivity_cluster_t *p,
        int port,
        message_handler_t *mh,
        int client_port) THROWS_NOTHING :

    parent(p), message_handler(mh),

    /* This sets `parent->current_run` to `this`. It's necessary to do it in the
    constructor of a subfield rather than in the body of the `run_t` constructor
    because `parent->current_run` needs to be set before `connection_to_ourself`
    is constructed. Otherwise, something could try to send a message to ourself
    in response to a connection notification from the constructor for
    `connection_to_ourself`, and that would be a problem. */
    register_us_with_parent(&parent->current_run, this),

    /* This constructor makes an entry for us in `routing_table`. The destructor
    will remove the entry. */
    routing_table_entry_for_ourself(&routing_table, parent->me, peer_address_t(ip_address_t::us(), port)),

    /* The `connection_entry_t` constructor takes care of putting itself in the
    `connection_map` on each thread and notifying any listeners that we're now
    connected to ourself. The destructor will remove us from the
    `connection_map` and again notify any listeners. */
    connection_to_ourself(this, parent->me, NULL, routing_table[parent->me]),

    /* The local port to use when connecting to the cluster port of peers */
    cluster_client_port(client_port),

    listener(port, boost::bind(
        &connectivity_cluster_t::run_t::on_new_connection,
        this,
        _1,
        auto_drainer_t::lock_t(&drainer)
        ))
{
    parent->assert_thread();
}

void connectivity_cluster_t::run_t::join(peer_address_t address) THROWS_NOTHING {
    parent->assert_thread();
    coro_t::spawn_now(boost::bind(
        &connectivity_cluster_t::run_t::join_blocking,
        this,
        address,
        /* We don't know what `peer_id_t` the peer has until we connect to it */
        boost::none,
        auto_drainer_t::lock_t(&drainer)
        ));
}

connectivity_cluster_t::run_t::connection_entry_t::connection_entry_t(run_t *p, peer_id_t id, streamed_tcp_conn_t *c, peer_address_t a) THROWS_NOTHING :
    conn(c), address(a), session_id(generate_uuid()),
    parent(p), peer(id),
    drainers(new boost::scoped_ptr<auto_drainer_t>[get_num_threads()])
{
    /* This can be created and destroyed on any thread. */
    pmap(get_num_threads(),
        boost::bind(&connectivity_cluster_t::run_t::connection_entry_t::install_this, this, _1));
}

connectivity_cluster_t::run_t::connection_entry_t::~connection_entry_t() THROWS_NOTHING {
    pmap(get_num_threads(),
        boost::bind(&connectivity_cluster_t::run_t::connection_entry_t::uninstall_this, this, _1));

    /* `uninstall_this()` destroys the `auto_drainer_t`, so nothing can be
    holding the `send_mutex`. */
    rassert(!send_mutex.is_locked());
}

static void ping_connection_watcher(peer_id_t peer,
        const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb) THROWS_NOTHING {
    if (connect_cb_and_disconnect_cb.first) {
        connect_cb_and_disconnect_cb.first(peer);
    }
}

void connectivity_cluster_t::run_t::connection_entry_t::install_this(int target_thread) THROWS_NOTHING {
    on_thread_t switcher(target_thread);
    thread_info_t *ti = parent->parent->thread_info.get();
    drainers[get_thread_id()].reset(new auto_drainer_t);
    {
        ASSERT_FINITE_CORO_WAITING;
        rwi_lock_assertion_t::write_acq_t acq(&ti->lock);
        rassert(ti->connection_map.find(peer) == ti->connection_map.end());
        ti->connection_map[peer] =
            std::make_pair(this, auto_drainer_t::lock_t(drainers[get_thread_id()].get()));
        ti->publisher.publish(boost::bind(&ping_connection_watcher, peer, _1));
    }
}

static void ping_disconnection_watcher(peer_id_t peer,
        const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb) THROWS_NOTHING {
    if (connect_cb_and_disconnect_cb.second) {
        connect_cb_and_disconnect_cb.second(peer);
    }
}

void connectivity_cluster_t::run_t::connection_entry_t::uninstall_this(int target_thread) THROWS_NOTHING {
    on_thread_t switcher(target_thread);
    thread_info_t *ti = parent->parent->thread_info.get();
    {
        ASSERT_FINITE_CORO_WAITING;
        rwi_lock_assertion_t::write_acq_t acq(&ti->lock);
        rassert(ti->connection_map[peer].first == this);
        ti->connection_map.erase(peer);
        ti->publisher.publish(boost::bind(&ping_disconnection_watcher, peer, _1));
    }
    drainers[get_thread_id()].reset();
}

void connectivity_cluster_t::run_t::on_new_connection(boost::scoped_ptr<streamed_tcp_conn_t> &conn, auto_drainer_t::lock_t lock) THROWS_NOTHING {
    parent->assert_thread();
    handle(conn.get(), boost::none, boost::none, lock);
}

void connectivity_cluster_t::run_t::join_blocking(
        peer_address_t address,
        boost::optional<peer_id_t> expected_id,
        auto_drainer_t::lock_t drainer_lock) THROWS_NOTHING {
    parent->assert_thread();
    try {
        streamed_tcp_conn_t conn(address.ip, address.port, cluster_client_port);
        handle(&conn, expected_id, boost::optional<peer_address_t>(address), drainer_lock);
    } catch (tcp_conn_t::connect_failed_exc_t) {
        /* Ignore */
    }
}

static void close_conn(streamed_tcp_conn_t *c) THROWS_NOTHING {
    if (c->is_read_open()) {
        c->shutdown_read();
    }
    if (c->is_write_open()) {
        c->shutdown_write();
    }
}

void connectivity_cluster_t::run_t::handle(
        /* `conn` should remain valid until `handle()` returns. `handle()` does
        not take ownership of `conn`. */
        streamed_tcp_conn_t *conn,
        boost::optional<peer_id_t> expected_id,
        boost::optional<peer_address_t> expected_address,
        auto_drainer_t::lock_t drainer_lock) THROWS_NOTHING
{
    parent->assert_thread();

    /* Each side sends their own ID and address, then receives the other side's.
    */

    try {
        boost::archive::binary_oarchive sender(conn->get_ostream());
        sender << parent->me;
        sender << routing_table[parent->me];
    } catch (boost::archive::archive_exception) {
        /* We expect that sending can fail due to a network problem. If that
        happens, just ignore it. If sending fails for some other reason, then
        the programmer should learn about it, so we rethrow the exception. */
        if (!conn->is_write_open()) {
            return;
        } else {
            throw;
        }
    }

    peer_id_t other_id;
    peer_address_t other_address;
    try {
        boost::archive::binary_iarchive receiver(conn->get_istream());
        receiver >> other_id;
        receiver >> other_address;
    } catch (boost::archive::archive_exception) {
        if (!conn->is_read_open()) {
            return;
        } else {
            throw;
        }
    }

    /* Sanity checks */
    if (other_id == parent->me) {
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
    parent->assert_thread();

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
    bool we_are_leader = parent->me < other_id;

    // Just saying: Still on rpc listener thread, for
    // sending/receiving routing table
    parent->assert_thread();
    std::map<peer_id_t, peer_address_t> other_routing_table;

    if (we_are_leader) {

        std::map<peer_id_t, peer_address_t> routing_table_to_send;

        /* Critical section: we must check for conflicts and register ourself
        without the interference of any other connections. This ensures that
        any conflicts are resolved consistently. It also ensures that if we get
        two connections from different nodes, one will find out about the other.
        */
        {
            mutex_t::acq_t acq(&new_connection_mutex);

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
            mutex_t::acq_t acq(&new_connection_mutex);

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
    parent->assert_thread();

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
                    &connectivity_cluster_t::run_t::join_blocking, this,
                    it->second,
                    boost::optional<peer_id_t>(it->first),
                    drainer_lock));
            }
        }
    }

    // We could pick a better way to pick a better thread, our choice
    // now is hopefully a performance non-problem.
    int chosen_thread = rng.randint(get_num_threads());

    cross_thread_signal_t connection_thread_drain_signal(drainer_lock.get_drain_signal(), chosen_thread);

    rethread_streamed_tcp_conn_t unregister_conn(conn, INVALID_THREAD);
    on_thread_t conn_threader(chosen_thread);
    rethread_streamed_tcp_conn_t reregister_conn(conn, get_thread_id());

    // Make sure that if we're ordered to shut down, any pending read
    // or write gets interrupted.
    signal_t::subscription_t conn_closer(
        boost::bind(&close_conn, conn),
        &connection_thread_drain_signal);

    {
        /* `connection_entry_t` is the public interface of this coroutine. Its
        constructor registers it in the `connectivity_cluster_t`'s connection
        map and notifies any connect listeners. */
        connection_entry_t conn_structure(this, other_id, conn, other_address);

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

                std::stringstream stream(message, std::stringstream::in | std::stringstream::binary);
                message_handler->on_message(other_id, stream);
            }
        } catch (boost::archive::archive_exception) {
            /* The exception broke us out of the loop, and that's what we
            wanted. This could either be because we lost contact with the peer
            or because the cluster is shutting down and `close_conn()` got
            called. */

            guarantee(!conn->is_read_open(), "the connection was open for read, which means we had a boost archive exception");
        }

        /* The `conn_structure` destructor removes us from the connection map
        and notifies any disconnect listeners. */
    }
}

connectivity_cluster_t::connectivity_cluster_t() THROWS_NOTHING :
    me(peer_id_t(generate_uuid())),
    current_run(NULL)
    { }

connectivity_cluster_t::~connectivity_cluster_t() THROWS_NOTHING {
    rassert(!current_run);
}

peer_id_t connectivity_cluster_t::get_me() THROWS_NOTHING {
    return me;
}

std::set<peer_id_t> connectivity_cluster_t::get_peers_list() THROWS_NOTHING {
    std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> > *connection_map =
        &thread_info.get()->connection_map;
    std::set<peer_id_t> peers;
    for (std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> >::const_iterator it = connection_map->begin();
            it != connection_map->end(); it++) {
        peers.insert((*it).first);
    }
    return peers;
}

boost::uuids::uuid connectivity_cluster_t::get_connection_session_id(peer_id_t peer) THROWS_NOTHING {
    std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> > *connection_map =
        &thread_info.get()->connection_map;
    std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> >::iterator it =
        connection_map->find(peer);
    rassert(it != connection_map->end(), "You're trying to access the session "
        "ID for an unconnected peer. Note that we are not considered to be "
        "connected to ourself until after a connectivity_cluster_t::run_t "
        "has been created.");
    return (*it).second.first->session_id;
}

connectivity_service_t *connectivity_cluster_t::get_connectivity_service() THROWS_NOTHING {
    /* This is kind of silly. We need to implement it because
    `message_service_t` has a `get_connectivity_service()` method, and we are
    also the `connectivity_service_t` for our own `message_service_t`. */
    return this;
}

void connectivity_cluster_t::send_message(peer_id_t dest, const boost::function<void(std::ostream&)> &writer) THROWS_NOTHING {
    // We could be on _any_ thread.

    rassert(!dest.is_nil());

    /* We currently write the message to a `stringstream`, then
       serialize that as a string. It's horribly inefficient, of course. */
    // TODO: If we don't do it this way, we (or the caller) will need
    // to worry about having the writer run on the connection thread.
    std::stringstream buffer(std::ios_base::out | std::stringstream::binary);
    {
        ASSERT_FINITE_CORO_WAITING;
        writer(buffer);
    }

#ifdef CLUSTER_MESSAGE_DEBUGGING
    std::cerr << "from " << me << " to " << dest << std::endl;
    print_hd(buffer.str().data(), 0, buffer.str().size());
#endif

#ifndef NDEBUG
    /* We're allowed to block indefinitely, but it's tempting to write code on
    the assumption that we won't. This might catch some programming errors. */
    if (debug_rng.randint(10) == 0) {
        nap(10);
    }
#endif

    /* Find the connection entry */
    run_t::connection_entry_t *conn_structure;
    auto_drainer_t::lock_t conn_structure_lock;
    {
        std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> > *connection_map =
            &thread_info.get()->connection_map;
        std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> >::const_iterator it =
            connection_map->find(dest);
        if (it == connection_map->end()) {
            /* We don't currently have access to this peer. Our policy is to not
            notify the sender when a message cannot be transmitted (since this
            is not always possible). So just return. */
            return;
        }
        conn_structure = (*it).second.first;
        conn_structure_lock = (*it).second.second;
    }

    if (conn_structure->conn == NULL) {
        /* We're sending a message to ourself */
        rassert(dest == me);
        // We could be on any thread here! Oh no!
        std::stringstream buffer2(buffer.str(), std::stringstream::in | std::stringstream::binary);
        current_run->message_handler->on_message(me, buffer2);

    } else {
        rassert(dest != me);
        on_thread_t threader(conn_structure->conn->home_thread());

        /* Acquire the send-mutex so we don't collide with other things trying
        to send on the same connection. */
        mutex_t::acq_t acq(&conn_structure->send_mutex);

        try {
            boost::archive::binary_oarchive sender(conn_structure->conn->get_ostream());
            std::string buffer_str = buffer.str();
            sender << buffer_str;
        } catch (boost::archive::archive_exception) {
            /* Close the other half of the connection to make sure that
            `connectivity_cluster_t::run_t::handle()` notices that something is
            up */
            if (conn_structure->conn->is_read_open()) {
                conn_structure->conn->shutdown_read();
            }
        }
    }
}

peer_address_t connectivity_cluster_t::get_peer_address(peer_id_t p) THROWS_NOTHING {
    std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> > *connection_map =
        &thread_info.get()->connection_map;
    std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> >::iterator it =
        connection_map->find(p);
    rassert(it != connection_map->end(), "You can only call get_peer_address() "
        "on a peer that we're currently connected to. Note that we're not "
        "considered to be connected to ourself until after the "
        "connectivity_cluster_t::run_t has been constructed.");
    return (*it).second.first->address;
}

rwi_lock_assertion_t *connectivity_cluster_t::get_peers_list_lock() THROWS_NOTHING {
    return &thread_info.get()->lock;
}

publisher_t< std::pair<
        boost::function<void(peer_id_t)>,
        boost::function<void(peer_id_t)>
        > > *connectivity_cluster_t::get_peers_list_publisher() THROWS_NOTHING {
    return thread_info.get()->publisher.get_publisher();
}
