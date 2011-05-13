
#ifndef __ARCH_LINUX_NETWORK_HPP__
#define __ARCH_LINUX_NETWORK_HPP__

#include "utils2.hpp"
#include <boost/scoped_ptr.hpp>
#include "arch/linux/event_queue.hpp"
#include "arch/address.hpp"
#include "concurrency/side_coro.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "concurrency/semaphore.hpp"
#include "concurrency/coro_pool.hpp"

/* linux_tcp_conn_t provides a nice wrapper around a TCP network connection. */

struct linux_tcp_conn_t :
    public home_thread_mixin_t,
    private linux_event_callback_t
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

    /* write_buffered() is like write(), but it might not send the data until flush_buffer*() or
    write() is called. Internally, it bundles together the buffered writes; this may improve
    performance. */
    void write_buffered(const void *buf, size_t size);
    void flush_buffer();   // Blocks until flush is done
    void flush_buffer_eventually();   // Blocks only if the queue is backed up

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

    scoped_fd_t sock;

    /* Overrides `home_thread_mixin_t`'s `rethread()` method */
    void rethread(int);

    /* Object that we use to watch for events. It's NULL when we are not registered on any
    thread, and otherwise is an object that's valid for the current thread. */
    linux_event_watcher_t *event_watcher;

    /* True if there is a pending read or write */
    bool read_in_progress, write_in_progress;

    /* These are pulsed if and only if the read/write end of the connection has been closed. */
    cond_t read_closed, write_closed;

    /* Holds data that we read from the socket but hasn't been consumed yet */
    std::vector<char> read_buffer;

    /* Reads up to the given number of bytes, but not necessarily that many. Simple wrapper around
    ::read(). Returns the number of bytes read or throws read_closed_exc_t. Bypasses read_buffer. */
    size_t read_internal(void *buffer, size_t size);

    /* Buffer we are currently filling up with data that we want to write. When it reaches a
    certain size, we push it onto `write_queue`. */
    std::vector<char> write_buffer;

    /* Schedules old write buffer's contents to be flushed and swaps in a fresh write buffer.
    Blocks until it can acquire the `write_queue_limiter` semaphore, but doesn't wait for
    data to be completely written. */
    void internal_flush_write_buffer();

    /* Used to queue up buffers to write. The functions in `write_queue` will all be
    `boost::bind()`s of the `perform_write()` function below. */
    unlimited_fifo_queue_t<boost::function<void()> > write_queue;

    /* This semaphore prevents the write queue from getting arbitrarily big. */
    semaphore_t write_queue_limiter;

    /* Used to actually perform the writes. Only has one coroutine in it. */
    coro_pool_t write_coro_pool;

    /* Used to actually perform a write. If the write end of the connection is open, then writes
    `size` bytes from `buffer` to the socket. */
    void perform_write(const void *buffer, size_t size);
};

/* The linux_tcp_listener_t is used to listen on a network port for incoming
connections. Create a linux_tcp_listener_t with some port and then call set_callback();
the provided callback will be called in a new coroutine every time something connects. */

class linux_tcp_listener_t : public linux_event_callback_t {
public:
    linux_tcp_listener_t(
        int port,
        boost::function<void(boost::scoped_ptr<linux_tcp_conn_t>&)> callback
        );
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
    boost::function<void(boost::scoped_ptr<linux_tcp_conn_t>&)> callback;

    /* accept_loop() runs in a separate coroutine. It repeatedly tries to accept
    new connections; when accept() blocks, then it waits for events from the
    event loop. When accept_loop_handler's destructor is called, accept_loop_handler
    stops accept_loop() by pulsing the signal. */
    boost::scoped_ptr<side_coro_handler_t> accept_loop_handler;
    void accept_loop(signal_t *);

    void handle(fd_t sock);

    /* event_watcher sends any error conditions to here */
    void on_event(int events);
    bool log_next_error;
};

#endif // __ARCH_LINUX_NETWORK_HPP__
