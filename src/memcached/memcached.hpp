#ifndef __MEMCACHED_MEMCACHED_HPP__
#define __MEMCACHED_MEMCACHED_HPP__

#include "memcached/send_buffer.hpp"
#include "conn_acceptor.hpp"

struct server_t;

struct txt_memcached_handler_t :
    public conn_handler_t,
    public net_conn_read_buffered_callback_t,
    public send_buffer_callback_t,
    public thread_message_t
{
    net_conn_t *conn;
    server_t *server;
    send_buffer_t send_buffer;

    txt_memcached_handler_t(conn_acceptor_t *acc, net_conn_t *, server_t *);
    ~txt_memcached_handler_t();

    // These just forward to send_buffer
    void write(const char *buffer, size_t bytes); 
    void writef(const char *format, ...);

    void quit();   // Called by conn_acceptor_t when the server is being shut down

    // Called to start the process of reading a new command off the socket
    void read_next_command();

    void on_thread_switch();
    void on_send_buffer_flush();
    void on_send_buffer_socket_closed();
    void on_net_conn_read_buffered(const char *buffer, size_t size);
    void on_net_conn_close();
};

#endif /* __MEMCACHED_MEMCACHED_HPP__ */
