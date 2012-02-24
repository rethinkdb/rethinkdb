
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
#include "containers/intrusive_list.hpp"

/* linux_raw_tcp_conn_t provides a thin wrapper around a TCP network connection.
   Use linux_buffered_tcp_conn_t if you need higher-level abstraction */

struct linux_raw_tcp_conn_t :
    public home_thread_mixin_t,
    private linux_event_callback_t
{
public:

    class write_callback_t {
    public:
        virtual ~write_callback_t() { };
        virtual void done() = 0;
    };

    struct connect_failed_exc_t : public std::exception {
        const char *what() throw () {
            return "Could not make connection";
        }
    };

    linux_raw_tcp_conn_t(const char *host, int port);
    linux_raw_tcp_conn_t(const ip_address_t &host, int port);

    // Note that is_read_open() and is_write_open() must both be false before the socket is destroyed.
    ~linux_raw_tcp_conn_t();

    /* Reading */

    struct read_closed_exc_t : public std::exception {
        const char *what() throw () {
            return "Network connection read end closed";
        }
    };

    // Returns when any data has been read from the socket.
    size_t read(void *buf, size_t size);
    // Returns when size bytes of data has been read from the socket.
    void read_exactly(void *buf, size_t size);
    // Returns immediately, after trying to read data from the socket
    size_t read_non_blocking(void *buf, size_t size);

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
    void write(const void *buf, size_t size, write_callback_t *callback);
    // This function will modify iov, abandon all hope ye who enter here
    void write_vectored(struct iovec *iov, size_t count, write_callback_t *callback);

    /* Call shutdown_write() to close the half of the pipe that goes from us to the peer. If there
    is a write currently happening, it will get write_closed_exc_t. */
    void shutdown_write();

    /* Returns false if the half of the pipe that goes from us to the peer has been closed. */
    bool is_write_open();

    /* Put a `perfmon_rate_monitor_t` here if you want to record stats on how fast data is being
    transmitted over the network. */
    perfmon_rate_monitor_t *write_perfmon;

private:
    friend class linux_buffered_tcp_conn_t;

    explicit linux_raw_tcp_conn_t(fd_t sock);   // Used by tcp_listener_t
    static fd_t connect_to(const char *host, int port);
    static fd_t connect_to(const ip_address_t &host, int port);

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
};

class linux_buffered_tcp_conn_t : public home_thread_mixin_t {
public:
    linux_buffered_tcp_conn_t(const char *host, int port);
    linux_buffered_tcp_conn_t(const ip_address_t &host, int port);

    void read_exactly(void *buf, size_t size); // Read until size bytes have been read
    void read_more_buffered(); // Reads available data
    const_charslice peek_read_buffer(); // Returns a const view of available data
    void pop_read_buffer(size_t len); // Removes len bytes from the read buffer

    void write(const void *buf, size_t size); // Batches buffers and may call write now or at a subsequent write/flush
    void flush_write_buffer(); // Writes all batched buffers

    // This returns the underlying 'raw' connection object, which will allow unbuffered reads/writes
    // This can only be called if the read buffer is empty
    linux_raw_tcp_conn_t& get_raw_connection();

    // These are pass-throughs to the underlying raw connection object
    void shutdown_write();
    bool is_write_open();
    void shutdown_read();
    bool is_read_open();
  
    typedef linux_raw_tcp_conn_t::write_callback_t write_callback_t;
    typedef linux_raw_tcp_conn_t::connect_failed_exc_t connect_failed_exc_t;
    typedef linux_raw_tcp_conn_t::read_closed_exc_t read_closed_exc_t;
    typedef linux_raw_tcp_conn_t::write_closed_exc_t write_closed_exc_t;

private:
    friend class linux_tcp_listener_t;
    explicit linux_buffered_tcp_conn_t(fd_t sock);   // Used by tcp_listener_t

    void rethread(int);
    char * get_read_buffer_start();
    char * get_read_buffer_end();

    static const uint32_t READ_BUFFER_RESIZE_FACTOR;
    static const uint32_t READ_BUFFER_MIN_UTILIZATION_FACTOR;
    static const size_t MIN_BUFFERED_READ_SIZE;
    static const size_t MIN_READ_BUFFER_SIZE;
    static const size_t MAX_WRITE_BUFFER_SIZE;

    size_t read_buffer_offset;
    std::vector<char> read_buffer;
    std::vector<char> write_buffer;

    linux_raw_tcp_conn_t conn;
};

/* The linux_tcp_listener_t is used to listen on a network port for incoming
connections. Create a linux_tcp_listener_t with some port and then call set_callback();
the provided callback will be called in a new coroutine every time something connects. */

class linux_tcp_listener_t : public linux_event_callback_t {
public:
    linux_tcp_listener_t(int port,
                         boost::function<void(boost::scoped_ptr<linux_buffered_tcp_conn_t>&)> callback);
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
    boost::function<void(boost::scoped_ptr<linux_buffered_tcp_conn_t>&)> callback;

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
