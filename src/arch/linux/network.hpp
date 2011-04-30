
#ifndef __ARCH_LINUX_NETWORK_HPP__
#define __ARCH_LINUX_NETWORK_HPP__

#include "utils2.hpp"
#include <boost/scoped_ptr.hpp>
#include "arch/linux/event_queue.hpp"
#include "arch/address.hpp"

/* Forward declaration to avoid circular header dependency */

struct cond_weak_ptr_t;

/* linux_tcp_conn_t provides a nice wrapper around a TCP network connection. */

struct linux_tcp_conn_t :
    public linux_event_callback_t
{
    friend class linux_tcp_listener_t;

public:
    struct connect_failed_exc_t : public std::exception {
        const char *what() throw () {
            return "Could not make connection";
        }
    };

    /* TODO: One of these forms should be replaced by the other. */
    linux_tcp_conn_t(const char *host, int port);
    linux_tcp_conn_t(const ip_address_t &host, int port);

    /* Reading */

    struct read_closed_exc_t : public std::exception {
        const char *what() throw () {
            return "Network connection read end closed";
        }
    };

    /* If you know beforehand how many bytes you want to read, use read() with a byte buffer.
    Returns when the buffer is full, or throws read_closed_exc_t. */
    void read(void *buf, size_t size);

    // If you don't know how many bytes you want to read, but still
    // masochistically want to handle buffering yourself.  Makes at
    // most one call to ::read(), reads some data or throws
    // read_closed_exc_t. read_some() is guaranteed to return at least
    // one byte of data unless it throws read_closed_exc_t.
    size_t read_some(void *buf, size_t size);

    // If you don't know how many bytes you want to read, use peek()
    // and then, if you're satisfied, pop what you've read, or if
    // you're unsatisfied, read_more_buffered() and then try again.
    // Note that you should always call peek() before calling
    // read_more_buffered(), because there might be leftover data in
    // the peek buffer that might be enough for you.
    const_charslice peek() const;
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

    /* Note that this only gets called to handle error-events. Read and write
    events are handled through the linux_event_watcher_t. */
    void on_event(int events);

    void on_shutdown_read();
    void on_shutdown_write();

    /* Reads up to the given number of bytes, but not necessarily that many. Simple wrapper around
    ::read(). Returns the number of bytes read or throws read_closed_exc_t. Bypasses read_buffer. */
    size_t read_internal(void *buffer, size_t size);

    /* Writes exactly the given number of bytes--like write(), but bypasses write_buffer. If the
    write end of the connection is closed, throws write_closed_exc_t. */
    void write_internal(const void *buffer, size_t size);

    scoped_fd_t sock;

    linux_event_watcher_t event_watcher;

    /* True if there is a pending read or write */
    bool read_in_progress, write_in_progress;

    /* If there is a pending read or write, these point to the cond_t that can be
    pulsed to interrupt the pending read or write.

    The reason for the boost::scoped_ptr<> is to avoid a circular dependency with
    cond_weak_ptr_t. */
    boost::scoped_ptr<cond_weak_ptr_t> read_cond_watcher, write_cond_watcher;

    // True when the half of the connection has been shut down but the linux_tcp_conn_t has not
    // been deleted yet
    bool read_was_shut_down;
    bool write_was_shut_down;

    /* Holds data that we read from the socket but hasn't been consumed yet */
    std::vector<char> read_buffer;

    /* Holds data that was given to us but we haven't sent to the kernel yet */
    std::vector<char> write_buffer;
};

/* The linux_tcp_listener_t is used to listen on a network port for incoming
connections. Create a linux_tcp_listener_t with some port and then call set_callback();
the provided callback will be called in a new coroutine every time something connects. */

struct linux_tcp_listener_callback_t {
    // Feel free to take ownership of the connection and have it deleted yourself.
    virtual void on_tcp_listener_accept(boost::scoped_ptr<linux_tcp_conn_t>& conn) = 0;
    virtual ~linux_tcp_listener_callback_t() {}
};

class linux_tcp_listener_t : public linux_event_callback_t {
public:
    explicit linux_tcp_listener_t(int port);
    void set_callback(linux_tcp_listener_callback_t *cb);
    ~linux_tcp_listener_t();

    // The constructor can throw this exception
    struct address_in_use_exc_t :
        public std::exception
    {
        const char *what() throw () {
            return "The requested port is already in use.";
        }
    };

private:
    // The socket to listen for connections on
    scoped_fd_t sock;

    // Sentry representing our registration with event loop
    linux_event_watcher_t event_watcher;

    // The callback to call when we get a connection
    linux_tcp_listener_callback_t *callback;

    /* accept_loop() runs in a separate coroutine. It repeatedly tries to accept
    new connections; when accept() blocks, then it waits for events from the
    event loop. To shut down accept_loop(), ~linux_tcp_listener_t() sets
    *shutdown_signal to true and then signals accept_loop_cond. */

    void accept_loop();
    bool *shutdown_signal;

    // The boost::scoped_ptr<> is to avoid a circular header dependency
    boost::scoped_ptr<cond_weak_ptr_t> accept_loop_cond_watcher;
    void handle(fd_t sock);

    /* event_watcher sends any error conditions to here */
    void on_event(int events);
    bool log_next_error;
};

#endif // __ARCH_LINUX_NETWORK_HPP__
