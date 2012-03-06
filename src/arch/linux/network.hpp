#ifndef __ARCH_LINUX_NETWORK_HPP__
#define __ARCH_LINUX_NETWORK_HPP__

#include "errors.hpp"
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

    class write_callback_t : public intrusive_list_node_t<write_callback_t> {
    public:
        virtual ~write_callback_t() { };
        virtual void done(bool success) = 0;
    private:
        friend class linux_raw_tcp_conn_t;
        size_t position_in_stream;
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

    // Reading

    struct read_closed_exc_t : public std::exception {
        const char *what() throw () {
            return "Network connection read end closed";
        }
    };

    // Returns when any data has been read from the socket.
    size_t read(void *buf, size_t size) THROWS_ONLY(read_closed_exc_t);
    // Returns when size bytes of data has been read from the socket.
    void read_exactly(void *buf, size_t size) THROWS_ONLY(read_closed_exc_t);
    // Returns immediately, after trying to read data from the socket
    size_t read_non_blocking(void *buf, size_t size) THROWS_ONLY(read_closed_exc_t);

    /* Call shutdown_read() to close the half of the pipe that goes from the peer to us. If there
    is an outstanding read() or peek_until() operation, it will throw read_closed_exc_t. */
    void shutdown_read();

    // Returns false if the half of the pipe that goes from the peer to us has been closed.
    bool is_read_open();

    // Writing

    struct write_closed_exc_t : public std::exception {
        explicit write_closed_exc_t(int err_) : err(err_) {
        }
        const char *what() throw () {
            return "Network connection write end closed";
        }

        int err;
    };

    // The write functions will block until all the data has been put on the send queue
    // The callback will be called once the buffer is not needed anymore (which will not be
    //  immediate if we use a zerocopy write)
    void write(const void *buf, size_t size, write_callback_t *callback) THROWS_ONLY(write_closed_exc_t);
    // This function will modify iov, abandon all hope ye who enter here.
    // If the amount of data written is bigger than some threshold and callback is provided, it will
    // do the zero-copy write, and later notify the caller through the callback that the buffers can
    // be used again.
    void write_vectored(struct iovec *iov, size_t count, write_callback_t *callback) THROWS_ONLY(write_closed_exc_t);

    /* Call shutdown_write() to close the half of the pipe that goes from us to the peer. If there
    is a write currently happening, it will get write_closed_exc_t. */
    void shutdown_write();

    // Returns false if the half of the pipe that goes from us to the peer has been closed.
    bool is_write_open();

    /* Put a `perfmon_rate_monitor_t` here if you want to record stats on how fast data is being
    transmitted over the network. */
    perfmon_rate_monitor_t *write_perfmon;

private:
    friend class linux_buffered_tcp_conn_t;

    explicit linux_raw_tcp_conn_t(fd_t sock);   // Used by tcp_listener_t
    static fd_t connect_to(const char *host, int port);
    static fd_t connect_to(const ip_address_t &host, int port);

    static void timer_hook(void *instance);
    void timer_handle_callbacks();

    void initialize_vmsplice_pipes() THROWS_ONLY(write_closed_exc_t);

    // These functions return false if the respective side of the socket was closed
    void wait_for_epoll_in() THROWS_ONLY(read_closed_exc_t);
    void wait_for_epoll_out() THROWS_ONLY(write_closed_exc_t);
    void wait_for_pipe_epoll_out() THROWS_ONLY(write_closed_exc_t);

    // These functions are also used to subscribe/unsubscribe from the write callback timer
    void add_write_callback(write_callback_t *callback);
    void remove_write_callbacks(bool success, size_t current_position);

    // Internal write functions
    void advance_iov(struct iovec *&iov, size_t &count, size_t bytes_written);
    void write_zerocopy(struct iovec *iov, size_t count, size_t bytes_to_write, write_callback_t *callback) THROWS_ONLY(write_closed_exc_t);
    void write_copy(struct iovec *iov, size_t count, size_t bytes_to_write, write_callback_t *callback) THROWS_ONLY(write_closed_exc_t);

    /* Note that this only gets called to handle error-events. Read and write
    events are handled through the linux_event_watcher_t. */
    void on_event(int events);

    void on_shutdown_read();
    void on_shutdown_write();

    static const size_t ZEROCOPY_THRESHOLD;
    scoped_fd_t sock;

    scoped_fd_t read_vmsplice_pipe;
    scoped_fd_t write_vmsplice_pipe;

    // Overrides `home_thread_mixin_t`'s `rethread()` method
    void rethread(int);

    /* Object that we use to watch for events. It's NULL when we are not registered on any
    thread, and otherwise is an object that's valid for the current thread. */
    linux_event_watcher_t *event_watcher;

    // True if there is a pending read or write
    bool read_in_progress, write_in_progress;

    // These are pulsed if and only if the read/write end of the connection has been closed.
    cond_t read_closed, write_closed;

    timer_token_t *timer_token;
    size_t total_bytes_written;
    intrusive_list_t<write_callback_t> write_callback_list;
};

class linux_buffered_tcp_conn_t : public home_thread_mixin_t {
public:
    typedef linux_raw_tcp_conn_t::write_callback_t write_callback_t;
    typedef linux_raw_tcp_conn_t::connect_failed_exc_t connect_failed_exc_t;
    typedef linux_raw_tcp_conn_t::read_closed_exc_t read_closed_exc_t;
    typedef linux_raw_tcp_conn_t::write_closed_exc_t write_closed_exc_t;

    linux_buffered_tcp_conn_t(const char *host, int port);
    linux_buffered_tcp_conn_t(const ip_address_t &host, int port);

    void read_exactly(void *buf, size_t size) THROWS_ONLY(read_closed_exc_t); // Read until size bytes have been read
    void read_more_buffered() THROWS_ONLY(read_closed_exc_t); // Reads available data
    const_charslice peek_read_buffer(); // Returns a const view of available data
    void pop_read_buffer(size_t len); // Removes len bytes from the read buffer

    void write(const void *buf, size_t size) THROWS_ONLY(write_closed_exc_t); // Batches buffers and may call write now or at a subsequent write/flush
    void write_vectored(struct iovec *iov, size_t count, write_callback_t *callback) THROWS_ONLY(write_closed_exc_t);
    void flush_write_buffer() THROWS_ONLY(write_closed_exc_t); // Writes all batched buffers

    // This returns the underlying 'raw' connection object, which will allow unbuffered reads/writes
    // This can only be called if the read buffer is empty
    linux_raw_tcp_conn_t& get_raw_connection() THROWS_ONLY(write_closed_exc_t);

    // These are pass-throughs to the underlying raw connection object
    void shutdown_write();
    bool is_write_open();
    void shutdown_read();
    bool is_read_open();

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
    linux_raw_tcp_conn_t conn;

    struct write_buffer_t {
        char *data;
        size_t size;
        size_t capacity;

        write_buffer_t(size_t size_);
        ~write_buffer_t();

        void clear();
        void put_back(const void *buf, size_t sz);
    } write_buffer;
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

    // event_watcher sends any error conditions to here
    void on_event(int events);
    bool log_next_error;
};

#endif // __ARCH_LINUX_NETWORK_HPP__
