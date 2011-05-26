#include "memcached/tcp_conn.hpp"
#include "memcached/memcached.hpp"

struct tcp_conn_memcached_interface_t : public memcached_interface_t, public home_thread_mixin_t {

    tcp_conn_memcached_interface_t(tcp_conn_t *c) : conn(c) { }

    tcp_conn_t *conn;

    void write(const thread_saver_t &saver, const char *buffer, size_t bytes) {
        try {
            // TODO: Stop needing this ensure_thread thing, so we
            // don't have to pass a thread_saver_t to everything.
            ensure_thread(saver);
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
                    size_t line_size = (char *)crlf_loc - (char *)sl.beg;

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
