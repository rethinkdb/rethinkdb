
#ifndef __ARCH_LINUX_NETWORK_HPP__
#define __ARCH_LINUX_NETWORK_HPP__

#include "utils2.hpp"
#include "arch/linux/event_queue.hpp"

/* linux_net_conn_t provides a nice wrapper around a network connection. */

struct linux_net_conn_read_external_callback_t
{
    virtual void on_net_conn_read_external() = 0;
    virtual void on_net_conn_close() = 0;
    virtual ~linux_net_conn_read_external_callback_t() { }
};

struct linux_net_conn_read_buffered_callback_t
{
    virtual void on_net_conn_read_buffered(const char *buffer, size_t size) = 0;
    virtual void on_net_conn_close() = 0;
    virtual ~linux_net_conn_read_buffered_callback_t() { }
};

struct linux_net_conn_write_external_callback_t
{
    virtual void on_net_conn_write_external() = 0;
    virtual void on_net_conn_close() = 0;
    virtual ~linux_net_conn_write_external_callback_t() { }
};

struct linux_net_conn_t :
    public linux_event_callback_t
{
    friend class linux_net_listener_t;

public:
    linux_net_conn_t(const char *host, int port);

    /* If you know beforehand how many bytes you want to read, use read_external() with a byte
    buffer. cb->on_net_conn_read_external() will be called when exactly that many bytes have been
    read. If the connection is closed, then cb->on_net_conn_close() will be called. */
    void read_external(void *buf, size_t size, linux_net_conn_read_external_callback_t *cb);

    /* If you don't know how many bytes you want to read, use read_buffered().
    cb->on_net_conn_read_buffered() will be called with some bytes. If there are enough bytes,
    call accept_buffer() with how many of the bytes you want to consume. If there are not enough,
    then return from cb->on_net_conn_read_buffered() without calling accept_buffer(), and the
    net_conn_t will later call on_net_conn_read_buffered() again with more bytes. If the connection
    is closed before an acceptable amount of data is read, then cb->on_net_conn_close() is called.
    This is useful for reading until you reach some delimiter, such as if you want to read until
    you hit a newline. */
    void read_buffered(linux_net_conn_read_buffered_callback_t *cb);
    void accept_buffer(size_t size);

    /* Call shutdown_read() to close the half of the pipe that goes from the peer to us. If there
    are any outstanding read_*() commands, then they will get on_net_conn_closed() called. Further
    calls to read_*() will fail. */
    void shutdown_read();

    /* Returns true if the half of the pipe that goes from the peer to us has been closed. */
    bool is_read_open();

    /* write_external() writes 'size' bytes from 'buf' to the socket and calls
    cb->on_net_conn_write_external() when it's done. If the connection is closed before all the
    bytes can be written, cb->on_net_conn_close() is called. */
    void write_external(const void *buf, size_t size, linux_net_conn_write_external_callback_t *cb);

    /* Call shutdown_write() to close the half of the pipe that goes from us to the peer. If there
    are any outstanding write_external() commands, they will get on_net_conn_closed() called. Further
    calls to write_* will fail. */
    void shutdown_write();

    /* Returns true if the half of the pipe that goes from us to the peer has been closed. */
    bool is_write_open();

    /* Note that closed_reading() and closed_writing() must both be true before the socket is
    destroyed. */
    ~linux_net_conn_t();

private:
    explicit linux_net_conn_t(fd_t sock);   // Used by net_listener_t
    fd_t sock;

    /* Before we are being watched by any event loop, registration_thread is -1. Once an
    event loop is watching us, registration_thread is its ID. */
    int registration_thread;
    void register_with_event_loop();

    void on_event(int events);
    bool *set_me_true_on_delete;   // In case we get deleted by a callback

    enum {
        read_mode_none,
        read_mode_external,
        read_mode_buffered
    } read_mode;

    // Used in read_mode_external
    char *external_read_buf;
    size_t external_read_size;
    linux_net_conn_read_external_callback_t *read_external_cb;
    void try_to_read_external_buf();

    // Used in read_mode_buffered
    std::vector<char> peek_buffer;
    linux_net_conn_read_buffered_callback_t *read_buffered_cb;
    bool in_read_buffered_cb;
    bool see_if_callback_is_satisfied();
    void put_more_data_in_peek_buffer();

    enum {
        write_mode_none,
        write_mode_external
    } write_mode;

    // Used in write_mode_external
    const char *external_write_buf;
    size_t external_write_size;
    linux_net_conn_write_external_callback_t *write_external_cb;
    void try_to_write_external_buf();

    // Called when one half of connection dies for any reason, either via shutdown_*() or by the
    // remote host
    void on_shutdown_read();
    void on_shutdown_write();

    // True when the half of the connection has been shut down but the linux_net_conn_t has not
    // been deleted yet
    bool read_was_shut_down;
    bool write_was_shut_down;
};

/* The linux_net_listener_t is used to listen on a network port for incoming
connections. Create a linux_net_listener_t with some port and then call set_callback();
the provided callback will be called every time something connects. */

struct linux_net_listener_callback_t {
    virtual void on_net_listener_accept(linux_net_conn_t *conn) = 0;
    virtual ~linux_net_listener_callback_t() {}
};

class linux_net_listener_t : public linux_event_callback_t {
public:
    explicit linux_net_listener_t(int port);
    void set_callback(linux_net_listener_callback_t *cb);
    ~linux_net_listener_t();
    
    bool defunct;

private:
    fd_t sock;
    linux_net_listener_callback_t *callback;
    
    void on_event(int events);
};

#endif // __ARCH_LINUX_NETWORK_HPP__
