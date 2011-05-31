#ifndef __MEMCACHED_MEMCACHED_HPP__
#define __MEMCACHED_MEMCACHED_HPP__

#include "arch/arch.hpp"
#include "data_provider.hpp"
#include "btree/value.hpp"
#include "store.hpp"
#include "concurrency/fifo_checker.hpp"

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
        const char *what() throw () {
            return "No more data available from txt_memcached handler.";
        }
    };
    virtual void read(void *,size_t) = 0;
    virtual void read_line(std::vector<char> *) = 0;
};

void handle_memcache(
    memcached_interface_t *interface,
    get_store_t *get_store,
    set_store_interface_t *set_store,
    int max_concurrent_queries_per_connection);

#endif /* __MEMCACHED_MEMCACHED_HPP__ */
