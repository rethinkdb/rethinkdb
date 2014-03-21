// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "memcached/tcp_conn.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/io/network.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "logger.hpp"
#include "memcached/parser.hpp"
#include "perfmon/perfmon.hpp"
#include "utils.hpp"

struct tcp_conn_memcached_interface_t : public memcached_interface_t, public home_thread_mixin_debug_only_t {
    explicit tcp_conn_memcached_interface_t(tcp_conn_t *c) : conn(c) { }

    tcp_conn_t *conn;

    void write(const char *buffer, size_t bytes, signal_t *interruptor) {
        try {
            assert_thread();
            conn->write_buffered(buffer, bytes, interruptor);
        } catch (const tcp_conn_write_closed_exc_t &) {
            /* Ignore */
        }
    }

    void write_unbuffered(const char *buffer, size_t bytes, signal_t *interruptor) {
        try {
            conn->write(buffer, bytes, interruptor);
        } catch (const tcp_conn_write_closed_exc_t &) {
            /* Ignore */
        }
    }

    void flush_buffer(signal_t *interruptor) {
        try {
            conn->flush_buffer(interruptor);
        } catch (const tcp_conn_write_closed_exc_t &) {
            /* Ignore errors; it's OK for the write end of the connection to be closed. */
        }
    }

    bool is_write_open() {
        return conn->is_write_open();
    }

    void read(void *buf, size_t nbytes, signal_t *interruptor) {
        try {
            conn->read(buf, nbytes, interruptor);
        } catch (const tcp_conn_read_closed_exc_t &) {
            throw no_more_data_exc_t();
        }
    }

    void read_line(std::vector<char> *dest, signal_t *interruptor) {
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
                    conn->pop(line_size + 2, interruptor);
                    return;
                } else if (sl.end - sl.beg > threshold) {
                    // If a malfunctioning client sends a lot of data without a
                    // CRLF, we cut them off.  (This doesn't apply to large values
                    // because they are read from the socket via a different
                    // mechanism.)  There are better ways to handle this
                    // situation.
                    logERR("Aborting connection %p because we got more than %zd bytes without a CRLF",
                            coro_t::self(), threshold);
                    conn->shutdown_read();
                    throw tcp_conn_read_closed_exc_t();
                }

                // Keep trying until we get a complete line.
                conn->read_more_buffered(interruptor);
            }

        } catch (const tcp_conn_read_closed_exc_t &) {
            throw no_more_data_exc_t();
        }
    }
};

void serve_memcache(tcp_conn_t *conn, namespace_interface_t<memcached_protocol_t> *nsi, memcached_stats_t *stats, signal_t *interruptor) {
    tcp_conn_memcached_interface_t interface(conn);
    handle_memcache(&interface, nsi, MAX_CONCURRENT_QUERIES_PER_CONNECTION, stats, interruptor);
}


memcache_listener_t::memcache_listener_t(const std::set<ip_address_t> &local_addresses,
                                         int _port,
                                         namespace_repo_t<memcached_protocol_t> *_ns_repo,
                                         uuid_u _namespace_id,
                                         perfmon_collection_t *_parent)
    : port(_port),
      namespace_id(_namespace_id),
      ns_repo(_ns_repo),
      next_thread(0),
      parent(_parent),
      stats(parent),
      tcp_listener(new repeated_nonthrowing_tcp_listener_t(local_addresses, port,
          boost::bind(&memcache_listener_t::handle, this, auto_drainer_t::lock_t(&drainer), _1)))
{
    tcp_listener->begin_repeated_listening_attempts();
}

signal_t *memcache_listener_t::get_bound_signal() {
    return tcp_listener->get_bound_signal();
}

memcache_listener_t::~memcache_listener_t() { }

void memcache_listener_t::handle(auto_drainer_t::lock_t keepalive, const scoped_ptr_t<tcp_conn_descriptor_t> &nconn) {
    assert_thread();

    block_pm_duration conn_timer(&stats.pm_conns);

    /* We will switch to another thread so there isn't too much load on the thread
    where the `memcache_listener_t` lives */
    threadnum_t chosen_thread = threadnum_t((next_thread++) % get_num_db_threads());

    /* Construct a cross-thread watcher so we will get notified on `chosen_thread`
    when a shutdown command is delivered on the main thread. */
    cross_thread_signal_t signal_transfer(keepalive.get_drain_signal(), chosen_thread);

    on_thread_t thread_switcher(chosen_thread);
    scoped_ptr_t<tcp_conn_t> conn;
    nconn->make_overcomplicated(&conn);

    /* `serve_memcache()` will continuously serve memcache queries on the given conn
    until the connection is closed. */
    try {
        namespace_repo_t<memcached_protocol_t>::access_t ns_access(ns_repo, namespace_id, &signal_transfer);
        serve_memcache(conn.get(), ns_access.get_namespace_if(), &stats, &signal_transfer);
    } catch (const interrupted_exc_t &ex) {
        // Interrupted, nothing to do but return
    }
}
