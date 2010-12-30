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

struct send_buffer_t
{
public:
    send_buffer_t(net_conn_t *conn);
    void write(size_t bytes, const char *buffer);
    void write_external(size_t bytes, const char *buffer, send_buffer_external_write_callback_t *cb);
    void flush(send_buffer_callback_t *cb);

private:
    net_conn_t *conn;

    std::vector<char> buffer;

    void do_write_external(size_t bytes, const char *buffer, send_buffer_external_write_callback_t *cb);
    void do_flush(send_buffer_callback_t *cb);

#ifndef NDEBUG
    enum {
        ready,
        flushing,
        writing
    } state;
#endif
};

#endif /* __MEMCACHED_SEND_BUFFER_HPP__ */
