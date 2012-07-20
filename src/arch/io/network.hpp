#ifndef ARCH_IO_NETWORK_HPP_
#define ARCH_IO_NETWORK_HPP_

#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <vector>
#include <stdexcept>
#include <string>

#include "utils.hpp"
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>

#include "config/args.hpp"
#include "containers/scoped.hpp"
#include "arch/address.hpp"
#include "arch/io/event_watcher.hpp"
#include "arch/io/io_utils.hpp"
#include "arch/runtime/event_queue.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "concurrency/semaphore.hpp"
#include "concurrency/coro_pool.hpp"
#include "containers/intrusive_list.hpp"
#include "perfmon/types.hpp"

/* linux_tcp_conn_t provides a disgusting wrapper around a TCP network connection. */

class linux_tcp_conn_t :
    public home_thread_mixin_t,
    private linux_event_callback_t {
public:
    friend class linux_nascent_tcp_conn_t;

    class connect_failed_exc_t : public std::exception {
    public:
        explicit connect_failed_exc_t(int en) : error(en) { }
        const char *what() const throw () {
            return strprintf("Could not make connection: %s", strerror(error)).c_str();
        }
        ~connect_failed_exc_t() throw () { }
        int error;
    };

    // NB. interruptor cannot be NULL.
    linux_tcp_conn_t(const ip_address_t &host, int port, signal_t *interruptor, int local_port = 0) THROWS_ONLY(connect_failed_exc_t, interrupted_exc_t);

    /* Reading */

    struct read_closed_exc_t : public std::exception {
        const char *what() const throw () {
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

    void read_more_buffered();

    /* Call shutdown_read() to close the half of the pipe that goes from the peer to us. If there
    is an outstanding read() or peek_until() operation, it will throw read_closed_exc_t. */
    void shutdown_read();

    /* Returns false if the half of the pipe that goes from the peer to us has been closed. */
    bool is_read_open();

    /* Writing */

    struct write_closed_exc_t : public std::exception {
        const char *what() const throw () {
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

    void writef(const char *format, ...) __attribute__ ((format (printf, 2, 3)));

    void flush_buffer();   // Blocks until flush is done
    void flush_buffer_eventually();   // Blocks only if the queue is backed up

    /* Call shutdown_write() to close the half of the pipe that goes from us to the peer. If there
    is a write currently happening, it will get write_closed_exc_t. */
    void shutdown_write();

    /* Returns false if the half of the pipe that goes from us to the peer has been closed. */
    bool is_write_open();

    /* Call to enable/disable `SO_KEEPALIVE` for this socket. First version
    enables and configures it; second version disables it. */
    /* TODO: This API is insufficient because there's no way to use it on a
    connection before `connect()` is called. */
    void set_keepalive(int idle_seconds, int try_interval_seconds, int try_count);
    void set_keepalive();

    /* Put a `perfmon_rate_monitor_t` here if you want to record stats on how fast data is being
    transmitted over the network. */
    perfmon_rate_monitor_t *write_perfmon;

    /* Note that is_read_open() and is_write_open() must both be false before the socket is
    destroyed. */
    ~linux_tcp_conn_t();

public:

    void rethread(int);

    int getsockname(ip_address_t *addr);
    int getpeername(ip_address_t *addr);

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

    static const size_t WRITE_QUEUE_MAX_SIZE = 128 * KILOBYTE;
    static const size_t WRITE_CHUNK_SIZE = 8 * KILOBYTE;

    /* Structs to avoid over-using dynamic allocation */
    struct write_buffer_t : public intrusive_list_node_t<write_buffer_t> {
        char buffer[WRITE_CHUNK_SIZE];
        size_t size;
    };

    struct write_queue_op_t : public intrusive_list_node_t<write_queue_op_t> {
        write_buffer_t *dealloc;
        const void *buffer;
        size_t size;
        cond_t *cond;
        auto_drainer_t::lock_t keepalive;
    };

    class write_handler_t :
        public coro_pool_t<write_queue_op_t*>::callback_t {
    public:
        explicit write_handler_t(linux_tcp_conn_t *parent_);
    private:
        linux_tcp_conn_t *parent;
        void coro_pool_callback(write_queue_op_t *operation, signal_t *interruptor);
    } write_handler;

    /* Lists of unused buffers, new buffers will be put on this list until needed again, reducing
       the use of dynamic memory.  TODO: decay over time? */
    intrusive_list_t<write_buffer_t> unused_write_buffers;
    intrusive_list_t<write_queue_op_t> unused_write_queue_ops;

    write_buffer_t * get_write_buffer();
    write_queue_op_t * get_write_queue_op();
    void release_write_buffer(write_buffer_t *buffer);
    void release_write_queue_op(write_queue_op_t *op);


    /* Schedules old write buffer's contents to be flushed and swaps in a fresh write buffer.
    Blocks until it can acquire the `write_queue_limiter` semaphore, but doesn't wait for
    data to be completely written. */
    void internal_flush_write_buffer();

    /* Used to queue up buffers to write. The functions in `write_queue` will all be
    `boost::bind()`s of the `perform_write()` function below. */
    unlimited_fifo_queue_t<write_queue_op_t*, intrusive_list_t<write_queue_op_t> > write_queue;

    /* This semaphore prevents the write queue from getting arbitrarily big. */
    semaphore_t write_queue_limiter;

    /* Used to actually perform the writes. Only has one coroutine in it, which will call the
    handle_write_queue callback when operations are ready */
    coro_pool_t<write_queue_op_t*> write_coro_pool;

    /* Buffer we are currently filling up with data that we want to write. When it reaches a
    certain size, we push it onto `write_queue`. */
    write_buffer_t* current_write_buffer;

    /* Used to actually perform a write. If the write end of the connection is open, then writes
    `size` bytes from `buffer` to the socket. */
    void perform_write(const void *buffer, size_t size);

    /* memcpy up to n bytes from read_buffer into dest. Returns the number of bytes
    copied. Then pop_read_buffer() can be used to remove the fetched bytes from the read buffer.
    */
    // TODO ! Recover that to make pop() fast again
    //size_t memcpy_from_read_buffer(void *buf, const size_t n);
    //void pop_read_buffer(const size_t n);

    /* To make pop() more efficient, we only actually erase part of the read_buffer
    when POP_THRESHOLD bytes can be popped of. Before that point, we accumulate
    the length of popped bytes in popped_bytes. */
    static const size_t POP_THRESHOLD = 1024;
    size_t popped_bytes;

    boost::scoped_ptr<auto_drainer_t> drainer;
};

class linux_nascent_tcp_conn_t {
public:
    ~linux_nascent_tcp_conn_t();

    // Must get called exactly once during lifetime of this object.
    // Call it on the thread you'll use the connection on.
    void ennervate(boost::scoped_ptr<linux_tcp_conn_t> *tcp_conn);

    void ennervate(scoped_ptr_t<linux_tcp_conn_t> *tcp_conn);

    // Must get called exactly once during lifetime of this object.
    // Call it on the thread you'll use the connection on.
    void ennervate(linux_tcp_conn_t **tcp_conn_out);

private:
    friend class linux_tcp_listener_t;

    explicit linux_nascent_tcp_conn_t(fd_t fd);

private:
    fd_t fd_;

    DISABLE_COPYING(linux_nascent_tcp_conn_t);
};

class linux_tcp_bound_socket_t {
public:
    explicit linux_tcp_bound_socket_t(int _port);
    ~linux_tcp_bound_socket_t();
    int get_port() const;
    MUST_USE fd_t release();
private:
    fd_t sock_fd;
    int port;
};

/* The linux_tcp_listener_t is used to listen on a network port for incoming
connections. Create a linux_tcp_listener_t with some port and then call set_callback();
the provided callback will be called in a new coroutine every time something connects. */

class linux_tcp_listener_t : public linux_event_callback_t {
public:
    linux_tcp_listener_t(int port,
                         boost::function<void(scoped_ptr_t<linux_nascent_tcp_conn_t>&)> callback);
    linux_tcp_listener_t(linux_tcp_bound_socket_t *bound_socket,
                         boost::function<void(scoped_ptr_t<linux_nascent_tcp_conn_t>&)> callback);
    ~linux_tcp_listener_t();

private:
    void initialize_internal();

    // The socket to listen for connections on
    scoped_fd_t sock;

    // Sentry representing our registration with event loop
    linux_event_watcher_t event_watcher;

    // The callback to call when we get a connection
    boost::function<void(scoped_ptr_t<linux_nascent_tcp_conn_t>&)> callback;

    /* accept_loop() runs in a separate coroutine. It repeatedly tries to accept
    new connections; when accept() blocks, then it waits for events from the
    event loop. */
    void accept_loop(auto_drainer_t::lock_t);
    boost::scoped_ptr<auto_drainer_t> accept_loop_drainer;

    void handle(fd_t sock);

    /* event_watcher sends any error conditions to here */
    void on_event(int events);
    bool log_next_error;
};

std::vector<std::string> get_ips();

#endif // ARCH_IO_NETWORK_HPP_
