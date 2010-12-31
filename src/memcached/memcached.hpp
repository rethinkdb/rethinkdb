#ifndef __MEMCACHED_MEMCACHED_HPP__
#define __MEMCACHED_MEMCACHED_HPP__

#include "memcached/send_buffer.hpp"
#include "conn_acceptor.hpp"
#include "concurrency/semaphore.hpp"
#include "concurrency/rwi_lock.hpp"

struct server_t;

struct txt_memcached_handler_t :
    public home_thread_mixin_t,
    public conn_handler_t,
    public net_conn_read_buffered_callback_t,
    public send_buffer_callback_t,
    public thread_message_t,
    public lock_available_callback_t
{
    net_conn_t *conn;
    server_t *server;
    send_buffer_t send_buffer;
    ticks_t start_time;   // Used for various perfmonning stuff
    
    // Used to limit number of concurrent noreply requests
    semaphore_t requests_out_sem;
    
    /* If a client sends a bunch of noreply requests and then a 'quit', we cannot delete ourself
    immediately because the noreply requests still hold the 'requests_out' semaphore. We use this
    lock to figure out when we can delete ourself. Each request acquires this lock in non-exclusive
    (read) mode, and when we want to shut down we acquire it in exclusive (write) mode. That way
    we don't shut down until everything that holds a reference to the semaphore is gone. */
    rwi_lock_t prevent_shutdown_lock;
    void shut_down();
    void on_lock_available();
    bool shutting_down;   // true while we are waiting to acquire main_rwi_lock in exclusive mode

    txt_memcached_handler_t(conn_acceptor_t *acc, net_conn_t *, server_t *);
    ~txt_memcached_handler_t();

    // These just forward to send_buffer
    void write(const char *buffer, size_t bytes); 
    void writef(const char *format, ...);

    void quit();   // Called by conn_acceptor_t when the server is being shut down

    // Called to start the process of reading a new command off the socket
    void read_next_command();

    // Used to implement throttling. Write requests should call begin_write_command() and
    // end_write_command() to make sure that not too many write requests are sent
    // concurrently.
    void begin_write_command(semaphore_available_callback_t *cb);
    void end_write_command();

    void on_thread_switch();
    void on_send_buffer_flush();
    void on_send_buffer_socket_closed();
    void on_net_conn_read_buffered(const char *buffer, size_t size);
    void on_net_conn_close();
};

#endif /* __MEMCACHED_MEMCACHED_HPP__ */
