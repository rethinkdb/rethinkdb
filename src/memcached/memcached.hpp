#ifndef __MEMCACHED_MEMCACHED_HPP__
#define __MEMCACHED_MEMCACHED_HPP__

#include "memcached/send_buffer.hpp"
#include "conn_acceptor.hpp"
#include "concurrency/semaphore.hpp"
#include "concurrency/rwi_lock.hpp"
#include "concurrency/cond_var.hpp"

struct server_t;

struct txt_memcached_handler_t :
    public home_thread_mixin_t,
    public conn_handler_t
{
    net_conn_t *conn;
    server_t *server;
    send_buffer_t send_buffer;
    
    // Used to limit number of concurrent noreply requests
    semaphore_t requests_out_sem;
    
    /* If a client sends a bunch of noreply requests and then a 'quit', we cannot delete ourself
    immediately because the noreply requests still hold the 'requests_out' semaphore. We use this
    lock to figure out when we can delete ourself. Each request acquires this lock in non-exclusive
    (read) mode, and when we want to shut down we acquire it in exclusive (write) mode. That way
    we don't shut down until everything that holds a reference to the semaphore is gone. */
    rwi_lock_t prevent_shutdown_lock;

    txt_memcached_handler_t(conn_acceptor_t *acc, net_conn_t *, server_t *);

    // These just forward to send_buffer
    void write(const char *buffer, size_t bytes); 
    void writef(const char *format, ...);

    void quit();   // Called by conn_acceptor_t when the server is being shut down

    // Used to implement throttling. Write requests should call begin_write_command() and
    // end_write_command() to make sure that not too many write requests are sent
    // concurrently.
    void begin_write_command();
    void end_write_command();

    void run();   // Main loop; meant to be run in a coroutine
};

#endif /* __MEMCACHED_MEMCACHED_HPP__ */
