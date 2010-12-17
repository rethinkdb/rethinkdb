#ifndef __MEMCACHED_SEND_BUFFER_HPP__
#define __MEMCACHED_SEND_BUFFER_HPP__

#include "arch/arch.hpp"

struct send_buffer_callback_t {
    virtual void on_send_buffer_flush() = 0;
    virtual void on_send_buffer_socket_closed() = 0;
};

struct send_buffer_t :
    public net_conn_write_external_callback_t
{
public:
    send_buffer_t(net_conn_t *conn);
    void write(size_t bytes, const char *buffer);
    void flush(send_buffer_callback_t *cb);

private:
    net_conn_t *conn;
    send_buffer_callback_t *flush_cb;
    bool writing;

    /* One of these points to buffers[0] and the other points to buffers[1]. */
    std::vector<char> *writing_buffer, *growing_buffer;
    std::vector<char> buffers[2];

    void on_net_conn_write_external();
    void on_net_conn_close();
    void write_growing_buffer();
};

#endif /* __MEMCACHED_SEND_BUFFER_HPP__ */
