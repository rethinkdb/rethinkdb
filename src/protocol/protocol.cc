#include "protocol/protocol.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "db_thread_info.hpp"
#include "perfmon.hpp"
#include <boost/bind.hpp>
#include <iostream>

void set_interface_from_dummy(protocol_listener_t *pl, namespace_interface_t<redis_protocol_t> *intface) {
    pl->set_interface(intface);
}

// TODO Redis: Possibly extra argument giving the protocol
protocol_listener_t::protocol_listener_t(int port, get_store_t *get_store, set_store_interface_t *set_store) :
    get_store(get_store), set_store(set_store),
    redis_interface(NULL),
    next_thread(0)
{
    /*
    // Determine the protocol to serve based on the port number
    switch(port) {
    case 11211: // Default memcached port
        break;
    case 6379: // Default redis port
        serve_func = serve_redis;
        break;
    case 6380: // Default redis port
        serve_func = serve_redis;
        break;
    default: // Default to memcached
        break;
    }
    */

    const int repli_factor = 1;
    std::vector<redis_protocol_t::region_t> shards;
    key_range_t key_range1(key_range_t::none, store_key_t(""),  key_range_t::open, store_key_t("n"));
    key_range_t key_range2(key_range_t::none, store_key_t("n"),  key_range_t::open, store_key_t(""));
    shards.push_back(key_range1);
    shards.push_back(key_range2);


    // Stupid hack to set up the namespace interface for redis
    unittest::run_with_dummy_namespace_interface<redis_protocol_t>(shards, repli_factor, boost::bind(set_interface_from_dummy, this, _1));
    
    tcp_listener.reset(new tcp_listener_t(port, boost::bind(&protocol_listener_t::handle, this, _1)));
}

protocol_listener_t::~protocol_listener_t() {

    // Stop accepting new connections
    tcp_listener.reset();

    // Interrupt existing connections
    pulse_to_begin_shutdown.pulse();

    // Wait for existing connections to finish shutting down
    active_connection_drain_semaphore.drain();
}

class rethread_tcp_conn_t {
public:
    rethread_tcp_conn_t(tcp_conn_t *conn, int thread) : conn_(conn), old_thread_(conn->home_thread()), new_thread_(thread) {
        conn->rethread(thread);
        rassert(conn->home_thread() == thread);
    }

    ~rethread_tcp_conn_t() {
        rassert(conn_->home_thread() == new_thread_);
        conn_->rethread(old_thread_);
        rassert(conn_->home_thread() == old_thread_);
    }

private:
    tcp_conn_t *conn_;
    int old_thread_, new_thread_;

    DISABLE_COPYING(rethread_tcp_conn_t);
};

static void close_conn_if_open(tcp_conn_t *conn) {
    if (conn->is_read_open()) conn->shutdown_read();
}
 
void protocol_listener_t::handle(boost::scoped_ptr<tcp_conn_t> &conn) {
    assert_thread();

    drain_semaphore_t::lock_t dont_shut_down_yet(&active_connection_drain_semaphore);

    /* We will switch to another thread so there isn't too much load on the thread
    where the `memcache_listener_t` lives */

    int chosen_thread = (next_thread++) % get_num_db_threads();

    /* Construct a cross-thread watcher so we will get notified on `chosen_thread`
    when a shutdown command is delivered on the main thread. */
    cross_thread_signal_t signal_transfer(&pulse_to_begin_shutdown, chosen_thread);

    /* Switch to the other thread. We use the `rethread_t` objects to unregister
    the conn with the event loop on this thread and to reregister it with the
    event loop on the new thread, then do the reverse when we switch back. */
    rethread_tcp_conn_t unregister_conn(conn.get(), INVALID_THREAD);
    on_thread_t thread_switcher(chosen_thread);
    rethread_tcp_conn_t reregister_conn(conn.get(), get_thread_id());

    /* Set up an object that will close the network connection when a shutdown signal
    is delivered */
    signal_t::subscription_t conn_closer(boost::bind(&close_conn_if_open, conn.get()));
    if (signal_transfer.is_pulsed()) close_conn_if_open(conn.get());
    else conn_closer.resubscribe(&signal_transfer);

    /* `serve_memcache()` will continuously serve memcache queries on the given conn
    until the connection is closed. */
    serve_redis(conn.get(), get_store, set_store, redis_interface);
}
