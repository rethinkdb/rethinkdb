#ifndef MEMCACHED_PARSER_HPP_
#define MEMCACHED_PARSER_HPP_

#include <vector>

#include "memcached/protocol.hpp"
#include "memcached/stats.hpp"
#include "protocol_api.hpp"

/* `handle_memcache()` handles memcache queries from the given `memcached_interface_t`,
sending the results to the same `memcached_interface_t`, until either SIGINT is sent to
the server or `memcache_interface_t::read()` or `memcache_interface_t::read_line()`
throws `no_more_data_exc_t`.

See `memcache/file.hpp` and `memcache/tcp_conn.hpp` for premade functions to handle
memcache traffic from either a file or a TCP connection. */

struct memcached_interface_t {

    virtual void write(const char *, size_t) = 0;
    virtual void write_unbuffered(const char *buffer, size_t bytes) = 0;

    virtual void flush_buffer() = 0;
    virtual bool is_write_open() = 0;

    struct no_more_data_exc_t : public std::exception {
        const char *what() const throw () {
            return "No more data available from txt_memcached handler.";
        }
    };
    virtual void read(void *, size_t) = 0;
    virtual void read_line(std::vector<char> *) = 0;

    virtual ~memcached_interface_t() { }
};

void handle_memcache(memcached_interface_t *interface,
                     namespace_interface_t<memcached_protocol_t> *nsi,
                     int max_concurrent_queries_per_connection,
                     memcached_stats_t *);

#endif /* MEMCACHED_PARSER_HPP_ */
