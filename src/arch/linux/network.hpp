
#ifndef __ARCH_LINUX_NETWORK_HPP__
#define __ARCH_LINUX_NETWORK_HPP__

#include "utils2.hpp"
#include "arch/linux/event_queue.hpp"

/* linux_net_conn_t provides a nice wrapper around a network connection. */

struct linux_net_conn_close_callback_t {
    virtual void on_net_conn_close() = 0;
};

struct linux_net_conn_write_callback_t :
    public virtual linux_net_conn_close_callback_t
{
    virtual void on_net_conn_write() = 0;
};

struct linux_net_conn_read_callback_t :
    public virtual linux_net_conn_close_callback_t
{
    virtual void on_net_conn_read() = 0;
};

struct linux_net_conn_t :
    public linux_event_callback_t
{
    friend class linux_net_listener_t;
    friend class linux_oldstyle_net_conn_t;

public:
    linux_net_conn_t(const char *host, int port);
    void read(void *buf, size_t size, linux_net_conn_read_callback_t *cb);
    void write(const void *buf, size_t size, linux_net_conn_write_callback_t *cb);
    ~linux_net_conn_t();

private:
    explicit linux_net_conn_t(fd_t sock);   // Used by listener
    fd_t sock;

    /* Before we are being watched by any event loop, registration_cpu is -1. Once an
    event loop is watching us, registration_cpu is its ID. */
    int registration_cpu;
    void register_with_event_loop();

    void on_event(int events);

    void *read_buf;
    size_t read_size;
    linux_net_conn_read_callback_t *read_cb;
    void pump_read();

    const void *write_buf;
    size_t write_size;
    linux_net_conn_write_callback_t *write_cb;
    void pump_write();

    bool *set_me_true_on_delete;
};

/* The linux_net_listener_t is used to listen on a network port for incoming
connections. Create a linux_net_listener_t with some port and then call set_callback();
the provided callback will be called every time something connects. */

struct linux_net_listener_callback_t {
    virtual void on_net_listener_accept(linux_net_conn_t *conn) = 0;
    virtual ~linux_net_listener_callback_t() {}
};

class linux_net_listener_t :
    public linux_event_callback_t
{

public:
    explicit linux_net_listener_t(int port);
    void set_callback(linux_net_listener_callback_t *cb);
    ~linux_net_listener_t();

private:
    fd_t sock;
    linux_net_listener_callback_t *callback;
    
    void on_event(int events);
};

/* The conn_fsm was written to expect a very low-level connection API that exposes the
events from epoll() and the read() and write() system calls with a minimum of wrapping.
We offer this as the linux_oldstyle_net_conn_t. */

struct linux_oldstyle_net_conn_callback_t {
    virtual void on_net_conn_readable() = 0;
    virtual void on_net_conn_writable() = 0;
    virtual void on_net_conn_close() = 0;
    virtual ~linux_oldstyle_net_conn_callback_t() {}
};

class linux_oldstyle_net_conn_t :
    public linux_event_callback_t
{
public:
    /* If you use this constructor, you should not touch the original
    linux_net_conn_t afterwards. */
    linux_oldstyle_net_conn_t(linux_net_conn_t *);

    void set_callback(linux_oldstyle_net_conn_callback_t *cb);
    ssize_t read_nonblocking(void *buf, size_t count);
    ssize_t write_nonblocking(const void *buf, size_t count);
    ~linux_oldstyle_net_conn_t();

private:
    friend class linux_io_calls_t;
    friend class linux_net_listener_t;

    fd_t sock;
    linux_oldstyle_net_conn_callback_t *callback;
    bool *set_me_true_on_delete;   // So we can tell if a callback deletes the conn_fsm_t

    // We are implementing this for level-triggered mechanisms such as
    // poll, that will keep bugging us about the write when we don't
    // need it, and use up 100% of cpu
    bool registered_for_write_notifications;
    
    void on_event(int events);
};

#endif // __ARCH_LINUX_NETWORK_HPP__
