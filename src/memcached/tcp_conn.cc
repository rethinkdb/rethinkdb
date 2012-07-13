#include "memcached/tcp_conn.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/io/network.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "db_thread_info.hpp"
#include "logger.hpp"
#include "memcached/parser.hpp"
#include "perfmon/perfmon.hpp"

struct tcp_conn_memcached_interface_t : public memcached_interface_t, public home_thread_mixin_t {
    explicit tcp_conn_memcached_interface_t(tcp_conn_t *c) : conn(c) { }

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
                    logERR("Aborting connection %p because we got more than %ld bytes without a CRLF",
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

void serve_memcache(tcp_conn_t *conn, namespace_interface_t<memcached_protocol_t> *nsi, memcached_stats_t *stats) {
    tcp_conn_memcached_interface_t interface(conn);
    handle_memcache(&interface, nsi, MAX_CONCURRENT_QUERIES_PER_CONNECTION, stats);
}


memcache_listener_t::memcache_listener_t(int _port, namespace_interface_t<memcached_protocol_t> *namespace_if_, perfmon_collection_t *_parent)
    : port(_port), namespace_if(namespace_if_),
      next_thread(0),
      tcp_listener(new tcp_listener_t(port, boost::bind(&memcache_listener_t::handle,
                                                        this, auto_drainer_t::lock_t(&drainer), _1))),
      parent(_parent),
      stats(parent)
{ }

memcache_listener_t::~memcache_listener_t() { }

class memcache_conn_closing_subscription_t : public signal_t::subscription_t {
public:
    explicit memcache_conn_closing_subscription_t(tcp_conn_t *conn) : conn_(conn) { }

    virtual void run() {
        if (conn_->is_read_open()) {
            conn_->shutdown_read();
        }
    }
private:
    tcp_conn_t *conn_;
    DISABLE_COPYING(memcache_conn_closing_subscription_t);
};

void memcache_listener_t::handle(auto_drainer_t::lock_t keepalive, const boost::scoped_ptr<nascent_tcp_conn_t> &nconn) {
    assert_thread();

    block_pm_duration conn_timer(&stats.pm_conns);

    /* We will switch to another thread so there isn't too much load on the thread
    where the `memcache_listener_t` lives */
    //int chosen_thread = (next_thread++) % get_num_db_threads();
    int chosen_thread = get_thread_id();

    /* Construct a cross-thread watcher so we will get notified on `chosen_thread`
    when a shutdown command is delivered on the main thread. */
    cross_thread_signal_t signal_transfer(keepalive.get_drain_signal(), chosen_thread);

    on_thread_t thread_switcher(chosen_thread);
    boost::scoped_ptr<tcp_conn_t> conn;
    nconn->ennervate(&conn);

    /* Set up an object that will close the network connection when a shutdown signal
    is delivered */
    memcache_conn_closing_subscription_t conn_closer(conn.get());
    conn_closer.reset(&signal_transfer);

    /* `serve_memcache()` will continuously serve memcache queries on the given conn
    until the connection is closed. */
    serve_memcache(conn.get(), namespace_if, &stats);
}
