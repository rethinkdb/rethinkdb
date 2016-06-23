// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_NETWORK_HPP_
#define ARCH_IO_NETWORK_HPP_

#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef ENABLE_TLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#ifdef _WIN32
#include "windows.hpp"
#else
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <functional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "arch/compiler.hpp"
#include "config/args.hpp"
#include "concurrency/interruptor.hpp"
#include "containers/lazy_erase_vector.hpp"
#include "containers/scoped.hpp"
#include "arch/address.hpp"
#include "arch/io/event_watcher.hpp"
#include "arch/io/io_utils.hpp"
#include "arch/io/openssl.hpp"
#include "arch/runtime/event_queue.hpp"
#include "arch/types.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "concurrency/semaphore.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/exponential_backoff.hpp"
#include "containers/intrusive_list.hpp"
#include "crypto/error.hpp"
#include "perfmon/types.hpp"

/* linux_tcp_conn_t provides a disgusting wrapper around a TCP network connection. */

class linux_tcp_conn_t :
    public home_thread_mixin_t,
    private linux_event_callback_t {
public:
    friend class linux_tcp_conn_descriptor_t;

    void enable_keepalive();

    class connect_failed_exc_t : public std::exception {
    public:
        explicit connect_failed_exc_t(int en) :
            error(en),
            info("Could not make connection: " + errno_string(error)) { }

        const char *what() const throw () {
            return info.c_str();
        }

        ~connect_failed_exc_t() throw () { }

        const int error;
        const std::string info;
    };

    // NB. interruptor cannot be nullptr.
    linux_tcp_conn_t(
        const ip_address_t &host,
        int port,
        signal_t *interruptor,
        int local_port = ANY_PORT)
        THROWS_ONLY(connect_failed_exc_t, interrupted_exc_t);

    /* Reading */

    /* If you know beforehand how many bytes you want to read, use read() with a
    byte buffer. Returns when the buffer is full, or throws tcp_conn_read_closed_exc_t.
    If `closer` is pulsed, throws `read_closed_exc_t` and also closes the read
    half of the connection. */
    void read(void *buf, size_t size, signal_t *closer)
        THROWS_ONLY(tcp_conn_read_closed_exc_t);

    /* This is a convenience function around `read_more_buffered`, `peek` and `pop` that
    behaved like a normal `read`, but uses buffering internally.
    In many cases you'll want to use this instead of `read`, especially when reading
    a lot of small values. */
    void read_buffered(void *buf, size_t size, signal_t *closer)
        THROWS_ONLY(tcp_conn_read_closed_exc_t);

    // If you don't know how many bytes you want to read, but still
    // masochistically want to handle buffering yourself.  Makes at
    // most one call to ::read(), reads some data or throws
    // read_closed_exc_t. read_some() is guaranteed to return at least
    // one byte of data unless it throws read_closed_exc_t.
    size_t read_some(void *buf, size_t size, signal_t *closer)
         THROWS_ONLY(tcp_conn_read_closed_exc_t);

    // If you don't know how many bytes you want to read, use peek()
    // and then, if you're satisfied, pop what you've read, or if
    // you're unsatisfied, read_more_buffered() and then try again.
    // Note that you should always call peek() before calling
    // read_more_buffered(), because there might be leftover data in
    // the peek buffer that might be enough for you.
    const_charslice peek() const THROWS_ONLY(tcp_conn_read_closed_exc_t);

    //you can also peek with a specific size (this is really just convenient
    //for some things and can in some cases avoid an unneeded copy
    const_charslice peek(size_t size, signal_t *closer)
        THROWS_ONLY(tcp_conn_read_closed_exc_t);

    void pop(size_t len, signal_t *closer) THROWS_ONLY(tcp_conn_read_closed_exc_t);

    void read_more_buffered(signal_t *closer) THROWS_ONLY(tcp_conn_read_closed_exc_t);

    /* Call shutdown_read() to close the half of the pipe that goes from the peer to us. If there
    is an outstanding read() or peek_until() operation, it will throw tcp_conn_read_closed_exc_t. */
    virtual void shutdown_read();

    /* Returns false if the half of the pipe that goes from the peer to us has been closed. */
    bool is_read_open() const;

    /* Writing */

    /* write() writes 'size' bytes from 'buf' to the socket and blocks until it
    is done. Throws tcp_conn_write_closed_exc_t if the write half of the pipe is closed
    before we can finish. If `closer` is pulsed, closes the write half of the
    pipe and throws `tcp_conn_write_closed_exc_t`. */
    void write(const void *buf, size_t size, signal_t *closer)
        THROWS_ONLY(tcp_conn_write_closed_exc_t);

    /* write_buffered() is like write(), but it might not send the data until
    flush_buffer*() or write() is called. Internally, it bundles together the
    buffered writes; this may improve performance. */
    void write_buffered(const void *buf, size_t size, signal_t *closer)
        THROWS_ONLY(tcp_conn_write_closed_exc_t);

    void writef(signal_t *closer, const char *format, ...)
        THROWS_ONLY(tcp_conn_write_closed_exc_t) ATTR_FORMAT(printf, 3, 4);

    // Blocks until flush is done
    void flush_buffer(signal_t *closer)
        THROWS_ONLY(tcp_conn_write_closed_exc_t);
    // Blocks only if the queue is backed up
    void flush_buffer_eventually(signal_t *closer)
        THROWS_ONLY(tcp_conn_write_closed_exc_t);

    /* Call shutdown_write() to close the half of the pipe that goes from us to the peer. If there
    is a write currently happening, it will get tcp_conn_write_closed_exc_t. */
    virtual void shutdown_write();

    /* Returns false if the half of the pipe that goes from us to the peer has been closed. */
    bool is_write_open() const;

    /* Put a `perfmon_rate_monitor_t` here if you want to record stats on how fast data is being
    transmitted over the network. */
    perfmon_rate_monitor_t *write_perfmon;

    virtual ~linux_tcp_conn_t() THROWS_NOTHING;

    virtual void rethread(threadnum_t thread);

    bool getpeername(ip_and_port_t *ip_and_port);

    event_watcher_t *get_event_watcher() {
        return event_watcher.get();
    }

protected:

    void on_shutdown_read();
    void on_shutdown_write();

    // Used by tcp_listener_t and any derived classes.
    explicit linux_tcp_conn_t(fd_t sock);

    // The underlying TCP socket file descriptor.
    scoped_fd_t sock;

    /* These are pulsed if and only if the read/write end of the connection has been closed. */
    cond_t read_closed, write_closed;

private:

    /* `read_op_wrapper_t` and `write_op_wrapper_t` are an attempt to factor out
    the boilerplate that would otherwise be at the top of every read- or write-
    related method on `linux_tcp_conn_t`. They check that the connection is
    open, handle `closer`, etc. */

    class read_op_wrapper_t : private signal_t::subscription_t {
    public:
        read_op_wrapper_t(linux_tcp_conn_t *p, signal_t *closer) : parent(p) {
            parent->assert_thread();
            rassert(!parent->read_in_progress);
            if (closer->is_pulsed()) {
                if (parent->is_read_open()) {
                    parent->shutdown_read();
                }
            } else {
                this->reset(closer);
            }
            if (!parent->is_read_open()) {
                throw tcp_conn_read_closed_exc_t();
            }
            parent->read_in_progress = true;
        }
        ~read_op_wrapper_t() {
            parent->read_in_progress = false;
        }
    private:
        void run() {
            if (parent->is_read_open()) {
                parent->shutdown_read();
            }
        }
        linux_tcp_conn_t *parent;
    };

    class write_op_wrapper_t : private signal_t::subscription_t {
    public:
        write_op_wrapper_t(linux_tcp_conn_t *p, signal_t *closer) : parent(p) {
            parent->assert_thread();
            rassert(!parent->write_in_progress);
            if (closer->is_pulsed()) {
                if (parent->is_write_open()) {
                    parent->shutdown_write();
                }
            } else {
                this->reset(closer);
            }
            if (!parent->is_write_open()) {
                throw tcp_conn_write_closed_exc_t();
            }
            parent->write_in_progress = true;
        }
        ~write_op_wrapper_t() {
            parent->write_in_progress = false;
        }
    private:
        void run() {
            if (parent->is_write_open()) {
                parent->shutdown_write();
            }
        }
        linux_tcp_conn_t *parent;
    };

    /* Note that this only gets called to handle error-events. Read and write
    events are handled through the event_watcher_t. */
    void on_event(int events);

    /* Object that we use to watch for events. It's NULL when we are not registered on any
    thread, and otherwise is an object that's valid for the current thread. */
    scoped_ptr_t<event_watcher_t> event_watcher;

    /* True if there is a pending read or write */
    bool read_in_progress, write_in_progress;

    /* Holds data that we read from the socket but hasn't been consumed yet */
    lazy_erase_vector_t<char> read_buffer;

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

    class write_handler_t : public coro_pool_callback_t<write_queue_op_t*> {
    public:
        explicit write_handler_t(linux_tcp_conn_t *_parent);
    private:
        linux_tcp_conn_t *parent;
        void coro_pool_callback(write_queue_op_t *operation, signal_t *interruptor);
    } write_handler;

    template <class T>
    class deleting_intrusive_list_t : public intrusive_list_t<T> {
    public:
        ~deleting_intrusive_list_t() {
            while (T *x = this->head()) {
                this->pop_front();
                delete x;
            }
        }
    };

    /* Lists of unused buffers, new buffers will be put on this list until needed again, reducing
       the use of dynamic memory.  TODO: decay over time? */
    deleting_intrusive_list_t<write_buffer_t> unused_write_buffers;
    deleting_intrusive_list_t<write_queue_op_t> unused_write_queue_ops;

    write_buffer_t * get_write_buffer();
    write_queue_op_t * get_write_queue_op();
    void release_write_buffer(write_buffer_t *buffer);
    void release_write_queue_op(write_queue_op_t *op);


    /* Schedules old write buffer's contents to be flushed and swaps in a fresh write buffer.
    Blocks until it can acquire the `write_queue_limiter` semaphore, but doesn't wait for
    data to be completely written. */
    void internal_flush_write_buffer();

    /* Used to queue up buffers to write. The functions in `write_queue` will all be
    `std::bind()`s of the `perform_write()` function below. */
    unlimited_fifo_queue_t<write_queue_op_t*, intrusive_list_t<write_queue_op_t> > write_queue;

    /* This semaphore prevents the write queue from getting arbitrarily big. */
    static_semaphore_t write_queue_limiter;

    /* Used to actually perform the writes. Only has one coroutine in it, which will call the
    handle_write_queue callback when operations are ready */
    coro_pool_t<write_queue_op_t *> write_coro_pool;

    /* Buffer we are currently filling up with data that we want to write. When it reaches a
    certain size, we push it onto `write_queue`. */
    scoped_ptr_t<write_buffer_t> current_write_buffer;

    scoped_ptr_t<auto_drainer_t> drainer;

    /* Reads up to the given number of bytes, but not necessarily that many. Simple
    wrapper around ::read(). Returns the number of bytes read or throws
    tcp_conn_read_closed_exc_t. Bypasses read_buffer. */
    virtual size_t read_internal(
        void *buffer, size_t size
    ) THROWS_ONLY(tcp_conn_read_closed_exc_t);

    /* Used to actually perform a write. If the write end of the connection is open, then
    writes `size` bytes from `buffer` to the socket. */
    virtual void perform_write(const void *buffer, size_t size);
};

#ifdef ENABLE_TLS

/* tls_conn_wrapper_t wraps a TLS connection. */
class tls_conn_wrapper_t {
public:
    explicit tls_conn_wrapper_t(SSL_CTX *tls_ctx) THROWS_ONLY(crypto::openssl_error_t);

    ~tls_conn_wrapper_t();

    void set_fd(fd_t sock) THROWS_ONLY(crypto::openssl_error_t);

    SSL *get() { return conn; }

private:
    SSL *conn;

    DISABLE_COPYING(tls_conn_wrapper_t);
};

class linux_secure_tcp_conn_t :
    public linux_tcp_conn_t {
public:

    friend class linux_tcp_conn_descriptor_t;

    // Client connection constructor.
    linux_secure_tcp_conn_t(
        SSL_CTX *tls_ctx, const ip_address_t &host, int port,
        signal_t *interruptor, int local_port = ANY_PORT
    ) THROWS_ONLY(connect_failed_exc_t, crypto::openssl_error_t, interrupted_exc_t);

    ~linux_secure_tcp_conn_t() THROWS_NOTHING;

    /* shutdown_read() and shutdown_write() just close the socket rather than performing
    the full TLS shutdown procedure, because they're assumed not to block.
    Our usual `shutdown` does block and it also assumes that no other operation is
    currently ongoing on the connection, which we can't guarantee here. */
    virtual void shutdown_read() { shutdown_socket(); }
    virtual void shutdown_write() { shutdown_socket(); }

    virtual void rethread(threadnum_t thread);

private:

    // Server connection constructor.
    linux_secure_tcp_conn_t(
        SSL_CTX *tls_ctx, fd_t _sock, signal_t *interruptor
    ) THROWS_ONLY(crypto::openssl_error_t, interrupted_exc_t);

    void perform_handshake(signal_t *interruptor) THROWS_ONLY(
        crypto::openssl_error_t, interrupted_exc_t);

    /* Reads up to the given number of bytes, but not necessarily that many. Simple
    wrapper around ::read(). Returns the number of bytes read or throws
    tcp_conn_read_closed_exc_t. Bypasses read_buffer. */
    virtual size_t read_internal(void *buffer, size_t size) THROWS_ONLY(
        tcp_conn_read_closed_exc_t);

    /* Used to actually perform a write. If the write end of the connection is open, then
    writes `size` bytes from `buffer` to the socket. */
    virtual void perform_write(const void *buffer, size_t size);

    void shutdown();
    void shutdown_socket();

    bool is_open() { return !closed.is_pulsed(); }

    tls_conn_wrapper_t conn;

    cond_t closed;
};

#endif /* ENABLE_TLS */

class linux_tcp_conn_descriptor_t {
public:
    ~linux_tcp_conn_descriptor_t();

    void make_server_connection(
        tls_ctx_t *tls_ctx, scoped_ptr_t<linux_tcp_conn_t> *tcp_conn, signal_t *closer)
        THROWS_ONLY(crypto::openssl_error_t, interrupted_exc_t);

    // Must get called exactly once during lifetime of this object.
    // Call it on the thread you'll use the server connection on.
    void make_server_connection(
        tls_ctx_t *tls_ctx, linux_tcp_conn_t **tcp_conn_out, signal_t *closer)
        THROWS_ONLY(crypto::openssl_error_t, interrupted_exc_t);

private:
    friend class linux_nonthrowing_tcp_listener_t;

    explicit linux_tcp_conn_descriptor_t(fd_t fd);

private:
    fd_t fd_;

    DISABLE_COPYING(linux_tcp_conn_descriptor_t);
};

/* The linux_nonthrowing_tcp_listener_t is used to listen on a network port for incoming
connections. Create a linux_nonthrowing_tcp_listener_t with some port and then call set_callback();
the provided callback will be called in a new coroutine every time something connects. */

class linux_nonthrowing_tcp_listener_t : private linux_event_callback_t {
public:
    linux_nonthrowing_tcp_listener_t(const std::set<ip_address_t> &bind_addresses, int _port,
        const std::function<void(scoped_ptr_t<linux_tcp_conn_descriptor_t> &)> &callback);

    ~linux_nonthrowing_tcp_listener_t();

    MUST_USE bool begin_listening();
    bool is_bound() const;
    int get_port() const;

protected:
    friend class linux_tcp_listener_t;
    friend class linux_tcp_bound_socket_t;

    void bind_sockets();

    // The callback to call when we get a connection
    std::function<void(scoped_ptr_t<linux_tcp_conn_descriptor_t> &)> callback;

private:
    static const uint32_t MAX_BIND_ATTEMPTS = 20;
    int init_sockets();

    /* accept_loop() runs in a separate coroutine. It repeatedly tries to accept
    new connections; when accept() blocks, then it waits for events from the
    event loop. */
    void accept_loop(auto_drainer_t::lock_t lock);

#ifdef _WIN32
    void accept_loop_single(const auto_drainer_t::lock_t &lock,
                            exponential_backoff_t backoff,
                            windows_event_watcher_t *event_watcher);
#else
    fd_t wait_for_any_socket(const auto_drainer_t::lock_t &lock);
#endif
    scoped_ptr_t<auto_drainer_t> accept_loop_drainer;

    void handle(fd_t sock);

    /* event_watcher sends any error conditions to here */
    void on_event(int events);

    // The selected local addresses to listen on, 'any' if empty
    std::set<ip_address_t> local_addresses;

    // The port we're asked to bind to
    int port;

    // Inidicates successful binding to a port
    bool bound;

    // The sockets to listen for connections on
    scoped_array_t<scoped_fd_t> socks;

    // The last socket to get a connection, used for round-robining
    size_t last_used_socket_index;

    // Sentries representing our registrations with the event loop, one per socket
    scoped_array_t<scoped_ptr_t<event_watcher_t> > event_watchers;

    bool log_next_error;
};

/* Used by the old style tcp listener */
class linux_tcp_bound_socket_t {
public:
    linux_tcp_bound_socket_t(const std::set<ip_address_t> &bind_addresses, int _port);
    int get_port() const;
private:
    friend class linux_tcp_listener_t;

    scoped_ptr_t<linux_nonthrowing_tcp_listener_t> listener;
};

/* Replicates old constructor-exception-throwing style for backwards compaitbility */
class linux_tcp_listener_t {
public:
    linux_tcp_listener_t(linux_tcp_bound_socket_t *bound_socket,
        const std::function<void(scoped_ptr_t<linux_tcp_conn_descriptor_t> &)> &callback);
    linux_tcp_listener_t(const std::set<ip_address_t> &bind_addresses, int port,
        const std::function<void(scoped_ptr_t<linux_tcp_conn_descriptor_t> &)> &callback);

    int get_port() const;

private:
    scoped_ptr_t<linux_nonthrowing_tcp_listener_t> listener;
};

/* Like a linux tcp listener but repeatedly tries to bind to its port until successful */
class linux_repeated_nonthrowing_tcp_listener_t {
public:
    linux_repeated_nonthrowing_tcp_listener_t(const std::set<ip_address_t> &bind_addresses, int port,
        const std::function<void(scoped_ptr_t<linux_tcp_conn_descriptor_t> &)> &callback);
    void begin_repeated_listening_attempts();

    signal_t *get_bound_signal();
    int get_port() const;

private:
    void retry_loop(auto_drainer_t::lock_t lock);

    linux_nonthrowing_tcp_listener_t listener;
    cond_t bound_cond;
    auto_drainer_t drainer;
};

std::vector<std::string> get_ips();

#endif // ARCH_IO_NETWORK_HPP_
