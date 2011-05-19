#ifndef __MEMCACHED_MEMCACHED_HPP__
#define __MEMCACHED_MEMCACHED_HPP__

#include "arch/arch.hpp"
#include "data_provider.hpp"
#include "btree/value.hpp"

struct get_store_t;
struct set_store_interface_t;
class order_source_t;

void serve_memcache(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store, order_source_t *order_source);

void import_memcache(std::string, set_store_interface_t *set_store, order_source_t *order_source);

/* Interface for txt_memcached_handler. This was created so I (jdoliner) could
 * make a dummy one for importation of memcached commands from a file */
class txt_memcached_handler_if {
public:
    txt_memcached_handler_if(get_store_t *get_store, set_store_interface_t *set_store)
        : get_store(get_store), set_store(set_store), requests_out_sem(MAX_CONCURRENT_QUERIES_PER_CONNECTION)
    { }

    get_store_t *get_store;
    set_store_interface_t *set_store;

    // Used to limit number of concurrent noreply requests
    semaphore_t requests_out_sem;

    /* If a client sends a bunch of noreply requests and then a 'quit', we cannot delete ourself
    immediately because the noreply requests still hold the 'requests_out' semaphore. We use this
    semaphore to figure out when we can delete ourself. */
    drain_semaphore_t drain_semaphore;

    virtual void write(const thread_saver_t& saver, const std::string& buffer) = 0;
    virtual void write(const thread_saver_t& saver, const char *buffer, size_t bytes) = 0;
    virtual void vwritef(const thread_saver_t& saver, const char *format, va_list args) = 0;
    virtual void writef(const thread_saver_t& saver, const char *format, ...) = 0;
    virtual void write_unbuffered(const char *buffer, size_t bytes) = 0;
    virtual void write_from_data_provider(const thread_saver_t& saver, data_provider_t *dp) = 0;
    virtual void write_value_header(const thread_saver_t& saver, const char *key, size_t key_size, mcflags_t mcflags, size_t value_size) = 0;
    virtual void write_value_header(const thread_saver_t& saver, const char *key, size_t key_size, mcflags_t mcflags, size_t value_size, cas_t cas) = 0;
    virtual void error(const thread_saver_t& saver) = 0;
    virtual void write_crlf(const thread_saver_t& saver) = 0;
    virtual void write_end(const thread_saver_t& saver) = 0;
    virtual void client_error(const thread_saver_t& saver, const char *format, ...) = 0;
    virtual void server_error(const thread_saver_t& saver, const char *format, ...) = 0;
    virtual void client_error_bad_command_line_format(const thread_saver_t& saver) = 0;
    virtual void client_error_bad_data(const thread_saver_t& saver) = 0;
    virtual void client_error_not_allowed(const thread_saver_t& saver, bool op_is_write) = 0;
    virtual void server_error_object_too_large_for_cache(const thread_saver_t& saver) = 0;

    virtual void begin_write_command() {
        drain_semaphore.acquire();
    }
    virtual void end_write_command() {
        drain_semaphore.release();
    }
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
