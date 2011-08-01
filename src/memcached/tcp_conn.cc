#include "memcached/tcp_conn.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/coroutines.hpp"
#include "memcached/memcached.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "db_thread_info.hpp"
#include "perfmon.hpp"
#include "logger.hpp"

struct tcp_conn_memcached_interface_t : public memcached_interface_t, public home_thread_mixin_t {

    tcp_conn_memcached_interface_t(tcp_conn_t *c) : conn(c) { }

    tcp_conn_t *conn;

    void write(const char *buffer, size_t bytes) {
        try {
            assert_thread();
            conn->write_buffered(buffer, bytes);
        } catch (tcp_conn_t::write_closed_exc_t) {
            /* Ignore */
        }
    }

    void write_unbuffered(const char *buffer, size_t bytes) {
        try {
            conn->write(buffer, bytes);
        } catch (tcp_conn_t::write_closed_exc_t) {
            /* Ignore */
        }
    }

    void flush_buffer() {
        try {
            conn->flush_buffer();
        } catch (tcp_conn_t::write_closed_exc_t) {
            /* Ignore errors; it's OK for the write end of the connection to be closed. */
        }
    }

    bool is_write_open() {
        return conn->is_write_open();
    }

    void read(void *buf, size_t nbytes) {
        try {
            conn->read(buf, nbytes);
        } catch(tcp_conn_t::read_closed_exc_t) {
            throw no_more_data_exc_t();
        }
    }

    void read_line(std::vector<char> *dest) {
        try {
            for (;;) {
                const_charslice sl = conn->peek();
                void *crlf_loc = memmem(sl.beg, sl.end - sl.beg, "\r\n", 2);
                ssize_t threshold = MEGABYTE;

                if (crlf_loc) {
                    // We have a valid line.
                    size_t line_size = reinterpret_cast<char *>(crlf_loc) - sl.beg;

                    dest->resize(line_size + 2);  // +2 for CRLF
                    memcpy(dest->data(), sl.beg, line_size + 2);
                    conn->pop(line_size + 2);
                    return;
                } else if (sl.end - sl.beg > threshold) {
                    // If a malfunctioning client sends a lot of data without a
                    // CRLF, we cut them off.  (This doesn't apply to large values
                    // because they are read from the socket via a different
                    // mechanism.)  There are better ways to handle this
                    // situation.
                    logERR("Aborting connection %p because we got more than %ld bytes without a CRLF\n",
                            coro_t::self(), threshold);
                    conn->shutdown_read();
                    throw tcp_conn_t::read_closed_exc_t();
                }

                // Keep trying until we get a complete line.
                conn->read_more_buffered();
            }

        } catch(tcp_conn_t::read_closed_exc_t) {
            throw no_more_data_exc_t();
        }
    }
};

void serve_memcache(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store) {
    tcp_conn_memcached_interface_t interface(conn);
    handle_memcache(&interface, get_store, set_store, MAX_CONCURRENT_QUERIES_PER_CONNECTION);
}

perfmon_duration_sampler_t pm_conns("conns", secs_to_ticks(600), false);

memcache_listener_t::memcache_listener_t(int port, get_store_t *get_store, set_store_interface_t *set_store) :
    get_store(get_store), set_store(set_store),
    next_thread(0)
{
    tcp_listener.reset(new tcp_listener_t(port, boost::bind(&memcache_listener_t::handle, this, _1)));
}

memcache_listener_t::~memcache_listener_t() {

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

void memcache_listener_t::handle(boost::scoped_ptr<tcp_conn_t> &conn) {
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
    signal_t::subscription_t conn_closer(boost::bind(&close_conn_if_open, conn.get()));
    if (signal_transfer.is_pulsed()) close_conn_if_open(conn.get());
    else conn_closer.resubscribe(&signal_transfer);

    /* `serve_memcache()` will continuously serve memcache queries on the given conn
    until the connection is closed. */
    serve_memcache(conn.get(), get_store, set_store);
}
