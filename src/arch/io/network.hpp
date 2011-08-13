#ifndef __ARCH_IO_NETWORK_HPP__
#define __ARCH_IO_NETWORK_HPP__

#include <vector>

#include "utils.hpp"
#include <boost/scoped_ptr.hpp>
#include "arch/runtime/event_queue.hpp"
#include "arch/io/io_utils.hpp"
#include "arch/address.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "concurrency/semaphore.hpp"
#include "concurrency/coro_pool.hpp"
#include "perfmon_types.hpp"
#include "arch/io/event_watcher.hpp"
#include <boost/iterator/iterator_facade.hpp>

#include <stdexcept>
#include <stdarg.h>
#include <unistd.h>

class side_coro_handler_t;

/* linux_tcp_conn_t provides a nice wrapper around a TCP network connection. */

class linux_tcp_conn_t :
    public home_thread_mixin_t,
    private linux_event_callback_t {
public:
    friend class linux_tcp_listener_t;

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

    //you can also peek with a specific size (this is really just convenient
    //for some things and can in some cases avoid an unneeded copy
    const_charslice peek(size_t size);

    void pop(size_t len);

    //pop to the position the iterator has been incremented to
    class iterator;
    void pop(iterator &);

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

    void vwritef(const char *format, va_list args) {
        char buffer[1000];
        size_t bytes = vsnprintf(buffer, sizeof(buffer), format, args);
        rassert(bytes < sizeof(buffer));
        write(buffer, bytes);
    }

    void writef(const char *format, ...)
        __attribute__ ((format (printf, 2, 3))) {
        va_list args;
        va_start(args, format);
        vwritef(format, args);
        va_end(args);
    }

    void flush_buffer();   // Blocks until flush is done
    void flush_buffer_eventually();   // Blocks only if the queue is backed up

    /* Call shutdown_write() to close the half of the pipe that goes from us to the peer. If there
    is a write currently happening, it will get write_closed_exc_t. */
    void shutdown_write();

    /* Returns false if the half of the pipe that goes from us to the peer has been closed. */
    bool is_write_open();

    /* Put a `perfmon_rate_monitor_t` here if you want to record stats on how fast data is being
    transmitted over the network. */
    perfmon_rate_monitor_t *write_perfmon;

    /* Note that is_read_open() and is_write_open() must both be false1 before the socket is
    destroyed. */
    ~linux_tcp_conn_t();

public:
    /* iterator always you to handle the stream of data off the
     * socket with iterators, the iterator handles all of the buffering itself. The
     * impetus for this class comes from wanting to use network connection's with
     * Boost's spirit parser */

    class iterator 
        : public boost::iterator_facade<iterator, const char, boost::forward_traversal_tag>
    {
    friend class linux_tcp_conn_t;
    private:
        linux_tcp_conn_t *source;
        bool end;
        size_t pos;
    private:
        int compare(iterator const& other) const;
    private:
    // boost iterator interface
        void increment();
        bool equal(iterator const& other);
        const char &dereference();
    public:
        iterator();
        iterator(linux_tcp_conn_t *, size_t);
        iterator(linux_tcp_conn_t *, bool);
        iterator(iterator const& );
        ~iterator();
        char operator*();
        void operator++();
        void operator++(int);
        bool operator==(iterator const &);
        bool operator!=(iterator const &);
        bool operator<(iterator const &);
    };

public:
    iterator begin();
    iterator end();

    // Changes the home_thread_mixin_t superclass's real_home_thread.
    void rethread(int);

private:
    explicit linux_tcp_conn_t(fd_t sock);   // Used by tcp_listener_t

    /* Note that this only gets called to handle error-events. Read and write
    events are handled through the linux_event_watcher_t. */
    void on_event(int events);

    void on_shutdown_read();
    void on_shutdown_write();

    scoped_fd_t sock;

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

#endif // __ARCH_IO_NETWORK_HPP__
