#ifndef __MEMCACHED_SEND_BUFFER_HPP__
#define __MEMCACHED_SEND_BUFFER_HPP__

#include "arch/arch.hpp"

struct send_buffer_callback_t {
    virtual void on_send_buffer_flush() = 0;
    virtual void on_send_buffer_socket_closed() = 0;
};

struct send_buffer_external_write_callback_t {
    virtual void on_send_buffer_write_external() = 0;
    virtual void on_send_buffer_socket_closed() = 0;
};

struct send_buffer_t :
    public net_conn_write_external_callback_t
{
public:
    send_buffer_t(net_conn_t *conn);
    void write(size_t bytes, const char *buffer);
    void write_external(size_t bytes, const char *buffer, send_buffer_external_write_callback_t *cb);
    void flush(send_buffer_callback_t *cb);

private:
    net_conn_t *conn;

    std::vector<char> buffer;
    send_buffer_callback_t *flush_cb;

    const char *external_write_buffer;
    size_t external_write_size;
    send_buffer_external_write_callback_t *external_write_cb;

    void on_net_conn_write_external();
    void on_net_conn_close();
};

#endif /* __MEMCACHED_SEND_BUFFER_HPP__ */
