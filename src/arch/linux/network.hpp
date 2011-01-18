
#ifndef __ARCH_LINUX_NETWORK_HPP__
#define __ARCH_LINUX_NETWORK_HPP__

#include "utils2.hpp"
#include "arch/linux/event_queue.hpp"

template<class value_t>
struct value_cond_t;

/* linux_tcp_conn_t provides a nice wrapper around a TCP network connection. */

struct linux_tcp_conn_t :
    public linux_event_callback_t
{
    friend class linux_tcp_listener_t;

public:
    linux_tcp_conn_t(const char *host, int port);

    /* Reading */

    struct read_closed_exc_t : public std::exception {
        const char *what() throw () {
            return "Network connection read end closed";
        }
    };

    /* If you know beforehand how many bytes you want to read, use read() with a byte buffer.
    Returns when the buffer is full, or throws read_closed_exc_t. */
    void read(void *buf, size_t size);

    // If you don't know how many bytes you want to read, use peek()
    // and then, if you're satisfied, pop what you've read, or if
    // you're unsatisfied, read_more_buffered() and then try again.
    struct bufslice { const char *buf; size_t len; bufslice(const char *b, size_t n) : buf(b), len(n) { } };
    bufslice peek() const;
    void pop(size_t len);

    void read_more_buffered();


    /* Call shutdown_read() to close the half of the pipe that goes from the peer to us. If there
    is an outstanding read() or peek_until() operation, it will throw read_closed_exc_t. */
    void shutdown_read();

    /* Returns false if the half of the pipe that goes from the peer to us has been closed. */
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
    is a write currently happening, it will get write_closed_exc_t. */
    void shutdown_write();

    /* Returns false if the half of the pipe that goes from us to the peer has been closed. */
    bool is_write_open();

    /* Note that is_read_open() and is_write_open() must both be false1 before the socket is
    destroyed. */
    ~linux_tcp_conn_t();

private:
    explicit linux_tcp_conn_t(fd_t sock);   // Used by tcp_listener_t
    fd_t sock;

    /* Before we are being watched by any event loop, registration_thread is -1. Once an
    event loop is watching us, registration_thread is its ID. */
    int registration_thread;
    void register_with_event_loop();

    void on_event(int events);

    /* If read_cond is non-NULL, it will be signalled with true if there is data on the read end
    and with false if the read end is closed. Same with write_cond and writing. */
    value_cond_t<bool> *read_cond, *write_cond;

    // True when the half of the connection has been shut down but the linux_tcp_conn_t has not
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

/* The linux_tcp_listener_t is used to listen on a network port for incoming
connections. Create a linux_tcp_listener_t with some port and then call set_callback();
the provided callback will be called every time something connects. */

struct linux_tcp_listener_callback_t {
    virtual void on_tcp_listener_accept(linux_tcp_conn_t *conn) = 0;
    virtual ~linux_tcp_listener_callback_t() {}
};

class linux_tcp_listener_t : public linux_event_callback_t {
public:
    explicit linux_tcp_listener_t(int port);
    void set_callback(linux_tcp_listener_callback_t *cb);
    ~linux_tcp_listener_t();
    
    bool defunct;

private:
    fd_t sock;
    linux_tcp_listener_callback_t *callback;
    
    void on_event(int events);
};

#endif // __ARCH_LINUX_NETWORK_HPP__
