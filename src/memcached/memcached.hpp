#ifndef __MEMCACHED_MEMCACHED_HPP__
#define __MEMCACHED_MEMCACHED_HPP__

#include "arch/arch.hpp"
#include "data_provider.hpp"
#include "btree/value.hpp"
#include "buffer_cache/sequence_group.hpp"

class get_store_t;
class set_store_interface_t;
class order_source_t;

void serve_memcache(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store, int n_slices, order_source_t *order_source);

void import_memcache(std::string, set_store_interface_t *set_store, int n_slices, order_source_t *order_source);

/* Interface for txt_memcached_handler. This was created so I (jdoliner) could
 * make a dummy one for importation of memcached commands from a file */
class txt_memcached_handler_if {
public:
    txt_memcached_handler_if(get_store_t *get_store, set_store_interface_t *set_store, int num_slices, int _max_concurrent_queries_per_connection = MAX_CONCURRENT_QUERIES_PER_CONNECTION)
        : get_store(get_store), set_store(set_store), seq_group(num_slices), max_concurrent_queries_per_connection(_max_concurrent_queries_per_connection)
    { }

    virtual ~txt_memcached_handler_if() { }

    get_store_t *get_store;
    set_store_interface_t *set_store;

    // The sequence group, so that the buffer cache or btree can make
    // sure that operations aren't reordered after they get to the
    // buffer cache.  (Specifically, when creating the transaction_t,
    // see sequence_group.hpp.)
    sequence_group_t seq_group;

    const int max_concurrent_queries_per_connection;

    virtual void write(const std::string& buffer) = 0;
    virtual void write(const char *buffer, size_t bytes) = 0;
    virtual void vwritef(const char *format, va_list args) = 0;
    virtual void writef(const char *format, ...) = 0;
    virtual void write_unbuffered(const char *buffer, size_t bytes) = 0;
    virtual void write_from_data_provider(data_provider_t *dp) = 0;
    virtual void write_value_header(const char *key, size_t key_size, mcflags_t mcflags, size_t value_size) = 0;
    virtual void write_value_header(const char *key, size_t key_size, mcflags_t mcflags, size_t value_size, cas_t cas) = 0;
    virtual void error() = 0;
    virtual void write_crlf() = 0;
    virtual void write_end() = 0;
    virtual void client_error(const char *format, ...) = 0;
    virtual void server_error(const char *format, ...) = 0;
    virtual void client_error_bad_command_line_format() = 0;
    virtual void client_error_bad_data() = 0;
    virtual void client_error_not_allowed(bool op_is_write) = 0;
    virtual void server_error_object_too_large_for_cache() = 0;

    virtual void flush_buffer() = 0;

    virtual bool is_write_open() = 0;
    virtual void read(void *,size_t) = 0;
    virtual void read_line(std::vector<char> *) = 0;

    struct no_more_data_exc_t : public std::exception {
        const char *what() throw () {
            return "No more data available from txt_memcached handler.";
        }
    };
};

#endif /* __MEMCACHED_MEMCACHED_HPP__ */
