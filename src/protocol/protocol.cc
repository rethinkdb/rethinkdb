#include "protocol/protocol.hpp"
#include "protocol/memcached/tcp_conn.hpp"
#include "protocol/memcached2/memcached_proto.hpp"
#include "protocol/redis/redis_proto.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "db_thread_info.hpp"
#include "perfmon.hpp"
#include <boost/bind.hpp>
#include <iostream>

// TODO Redis: Possibly extra argument giving the protocol
protocol_listener_t::protocol_listener_t(int port, get_store_t *get_store, set_store_interface_t *set_store) :
    get_store(get_store), set_store(set_store),
    next_thread(0)
{
    // Determine the protocol to serve based on the port number
    switch(port) {
    case 11211: // Default memcached port
        serve_func = serve_memcache;
        break;
    case 6379: // Default redis port
        serve_func = serve_redis;
        break;
    case 6380: // Default redis port
        serve_func = serve_redis;
        break;
    default: // Default to memcached
        serve_func = serve_memcache2;
        break;
    }
    
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

perfmon_duration_sampler_t pm_conns("conns", secs_to_ticks(600), false);

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
 
void protocol_listener_t::handle(boost::scoped_ptr<tcp_conn_t> &conn) {
    assert_thread();

    drain_semaphore_t::lock_t dont_shut_down_yet(&active_connection_drain_semaphore);

    block_pm_duration conn_timer(&pm_conns);

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
    struct conn_closer_t : public signal_t::waiter_t {
        tcp_conn_t *conn;
        signal_t *signal;
        conn_closer_t(tcp_conn_t *c, signal_t *s) : conn(c), signal(s) {
            if (signal->is_pulsed()) on_signal_pulsed();
            else signal->add_waiter(this);
        }
        void on_signal_pulsed() {
            if (conn->is_read_open()) conn->shutdown_read();
        }
        ~conn_closer_t() {
            if (!signal->is_pulsed()) signal->remove_waiter(this);
        }
    } conn_closer(conn.get(), &signal_transfer);

    /* `serve_memcache()` will continuously serve memcache queries on the given conn
    until the connection is closed. */
    //tcp_conn_streambuf_t conn_stream_buff(conn.get());
    //std::iostream conn_stream(&conn_stream_buff);
    serve_func(conn.get(), get_store, set_store);
}
