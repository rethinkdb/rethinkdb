#ifndef __MEMCACHED_MEMCACHED_HPP__
#define __MEMCACHED_MEMCACHED_HPP__

#include "arch/arch.hpp"
#include "data_provider.hpp"
#include "btree/value.hpp"

class get_store_t;
class set_store_interface_t;
class order_source_t;

void serve_memcache(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store, order_source_t *order_source);

void import_memcache(std::string, set_store_interface_t *set_store, order_source_t *order_source);

/* Interface for txt_memcached_handler. This was created so I (jdoliner) could
 * make a dummy one for importation of memcached commands from a file */
class txt_memcached_handler_if {
public:
    txt_memcached_handler_if(get_store_t *get_store, set_store_interface_t *set_store, int max_concurrent_queries_per_connection = MAX_CONCURRENT_QUERIES_PER_CONNECTION)
        : get_store(get_store), set_store(set_store), requests_out_sem(max_concurrent_queries_per_connection)
    { }

    get_store_t *get_store;
    set_store_interface_t *set_store;

    // Used to limit number of concurrent noreply requests
    semaphore_t requests_out_sem;

    /* If a client sends a bunch of noreply requests and then a 'quit', we cannot delete ourself
    immediately because the noreply requests still hold the 'requests_out' semaphore. We use this
    semaphore to figure out when we can delete ourself. */
    drain_semaphore_t drain_semaphore;

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

    // Used to implement throttling. Write requests should call begin_write_command() and
    // end_write_command() to make sure that not too many write requests are sent
    // concurrently.
    //
    // Note: these methods are implemented in the base class because it's
    // dangerous not to do them, and as far as I can see the memcached handlers
    // will need the same behavior for these functions. Of course things change
    // and if they is indeed a need for seperate behaviors a second virtual
    // function that is called in handle_memcache should also be created, these
    // should not be removed
    void begin_write_command() {
        drain_semaphore.acquire();
        requests_out_sem.co_lock();
    }
    void end_write_command() {
        drain_semaphore.release();
        requests_out_sem.unlock();
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
