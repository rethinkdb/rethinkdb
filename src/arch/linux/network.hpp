
#ifndef __ARCH_LINUX_NETWORK_HPP__
#define __ARCH_LINUX_NETWORK_HPP__

#include "utils2.hpp"
#include "arch/linux/event_queue.hpp"

template<class value_t>
struct value_cond_t;

/* linux_net_conn_t provides a nice wrapper around a TCP network connection. */

struct linux_net_conn_t :
    public linux_event_callback_t
{
    friend class linux_net_listener_t;

public:
    linux_net_conn_t(const char *host, int port);

    /* Reading */

    struct read_closed_exc_t : public std::exception {
        const char *what() throw () {
            return "Network connection read end closed";
        }
    };

    /* If you know beforehand how many bytes you want to read, use read() with a byte buffer.
    Returns when the buffer is full, or throws read_closed_exc_t. */
    void read(void *buf, size_t size);

    /* If you don't know how many bytes you want to read, use peek_until(). peek_until() will
    repeatedly call the given callback with larger and larger buffers until the callback returns
    true. If the connection is closed, throws read_closed_exc_t. */
    struct peek_callback_t {
        virtual bool check(const void *buf, size_t size) throw () = 0;
    };
    void peek_until(peek_callback_t *cb);

    /* Call shutdown_read() to close the half of the pipe that goes from the peer to us. If there
    is an outstanding read() or peek_until() operation, it will throw read_closed_exc_t. */
    void shutdown_read();

    /* Returns true if the half of the pipe that goes from the peer to us has been closed. */
    bool is_read_open();

    /* Writing */

    struct write_closed_exc_t : public std::exception {
        const char *what() throw () {
            return "Network connection write end closed";
        }
    };

    /* write() writes 'size' bytes from 'buf' to the socket and blocks until it is done. Throws
    write_closed_exc_t if the write end of the pipe is closed before we can finish. */
    void write(const void *buf, size_t size);

    /* write_buffered() is like write(), but it might not send the data until flush_buffer() or
    write() is called. The advantage of this is that sometimes you want to build a message
    piece-by-piece, but if you don't buffer it then Nagle's algorithm will screw with your
    latency. */
    void write_buffered(const void *buf, size_t size);
    void flush_buffer();

    /* Call shutdown_write() to close the half of the pipe that goes from us to the peer. If there
    are any outstanding write_external() commands, they will get on_net_conn_closed() called. Further
    calls to write_* will fail. */
    void shutdown_write();

    /* Returns true if the half of the pipe that goes from us to the peer has been closed. */
    bool is_write_open();

    /* Note that is_read_open() and is_write_open() must both be false1 before the socket is
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

    /* If read_cond is non-NULL, it will be signalled with true if there is data on the read end
    and with false if the read end is closed. Same with write_cond and writing. */
    value_cond_t<bool> *read_cond, *write_cond;

    // True when the half of the connection has been shut down but the linux_net_conn_t has not
    // been deleted yet
    bool read_was_shut_down;
    bool write_was_shut_down;

    void on_shutdown_read();
    void on_shutdown_write();

    /* Reads up to the given number of bytes, but not necessarily that many. Simple wrapper around
    ::read(). Returns the number of bytes read or throws read_closed_exc_t. Bypasses read_buffer. */
    size_t read_internal(void *buffer, size_t size);

    /* Holds data that we read from the socket but hasn't been consumed yet */
    std::vector<char> read_buffer;

    /* Writes exactly the given number of bytes--like write(), but bypasses write_buffer. If the
    write end of the connection is closed, throws write_closed_exc_t. */
    void write_internal(const void *buffer, size_t size);

    /* Holds data that was given to us but we haven't sent to the kernel yet */
    std::vector<char> write_buffer;
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
