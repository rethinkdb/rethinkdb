// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifdef _WIN32

// WINDOWS TODO

#else

#include "arch/io/network.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "utils.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/runtime.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "arch/timing.hpp"
#include "arch/types.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/exponential_backoff.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/printf_buffer.hpp"
#include "logger.hpp"
#include "perfmon/perfmon.hpp"

int connect_ipv4_internal(fd_t socket, int local_port, const in_addr &addr, int port) {
    struct sockaddr_in sa;
    socklen_t sa_len(sizeof(sa));
    memset(&sa, 0, sa_len);
    sa.sin_family = AF_INET;

    if (local_port != 0) {
        sa.sin_port = htons(local_port);
        sa.sin_addr.s_addr = INADDR_ANY;
        if (bind(socket, reinterpret_cast<sockaddr *>(&sa), sa_len) != 0)
            logWRN("Failed to bind to local port %d: %s", local_port, errno_string(get_errno()).c_str());
    }

    sa.sin_port = htons(port);
    sa.sin_addr = addr;

    int res;
    do {
        res = connect(socket, reinterpret_cast<sockaddr *>(&sa), sa_len);
    } while (res == -1 && get_errno() == EINTR);

    return res;
}

int connect_ipv6_internal(fd_t socket, int local_port, const in6_addr &addr, int port, uint32_t scope_id) {
    struct sockaddr_in6 sa;
    socklen_t sa_len(sizeof(sa));
    memset(&sa, 0, sa_len);
    sa.sin6_family = AF_INET6;

    if (local_port != 0) {
        sa.sin6_port = htons(local_port);
        sa.sin6_addr = in6addr_any;
        if (bind(socket, reinterpret_cast<sockaddr *>(&sa), sa_len) != 0)
            logWRN("Failed to bind to local port %d: %s", local_port, errno_string(get_errno()).c_str());
    }

    sa.sin6_port = htons(port);
    sa.sin6_addr = addr;
    sa.sin6_scope_id = scope_id;

    int res;
    do {
        res = connect(socket, reinterpret_cast<sockaddr *>(&sa), sa_len);
    } while (res == -1 && get_errno() == EINTR);

    return res;
}

int create_socket_wrapper(int address_family) {
    int res = socket(address_family, SOCK_STREAM, 0);
    if (res == INVALID_FD) {
        // Let the user know something is wrong - except in the case where
        // TCP doesn't support AF_INET6, which may be fairly common and spammy
        if (get_errno() != EAFNOSUPPORT || address_family == AF_INET) {
            logERR("Failed to create socket: %s", errno_string(get_errno()).c_str());
        }
        throw linux_tcp_conn_t::connect_failed_exc_t(get_errno());
    }
    return res;
}

// Network connection object
linux_tcp_conn_t::linux_tcp_conn_t(const ip_address_t &peer,
                                   int port,
                                   signal_t *interruptor,
                                   int local_port) THROWS_ONLY(connect_failed_exc_t, interrupted_exc_t) :
        write_perfmon(nullptr),
        sock(create_socket_wrapper(peer.get_address_family())),
        event_watcher(new linux_event_watcher_t(sock.get(), this)),
        read_in_progress(false), write_in_progress(false),
        read_buffer(IO_BUFFER_SIZE),
        write_handler(this),
        write_queue_limiter(WRITE_QUEUE_MAX_SIZE),
        write_coro_pool(1, &write_queue, &write_handler),
        current_write_buffer(get_write_buffer()),
        drainer(new auto_drainer_t) {
    guarantee_err(fcntl(sock.get(), F_SETFL, O_NONBLOCK) == 0, "Could not make socket non-blocking");

    if (local_port != 0) {
        // Set the socket to reusable so we don't block out other sockets from this port
        int reuse = 1;
        if (setsockopt(sock.get(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0)
            logWRN("Failed to set socket reuse to true: %s", errno_string(get_errno()).c_str());
    }
    {
        // Disable Nagle algorithm just as in the listener case
        int sockoptval = 1;
        int res = setsockopt(sock.get(), IPPROTO_TCP, TCP_NODELAY, &sockoptval, sizeof(sockoptval));
        guarantee_err(res != -1, "Could not set TCP_NODELAY option");
    }

    int res;
    if (peer.is_ipv4()) {
        res = connect_ipv4_internal(sock.get(), local_port, peer.get_ipv4_addr(), port);
    } else {
        res = connect_ipv6_internal(sock.get(), local_port, peer.get_ipv6_addr(), port,
                                    peer.get_ipv6_scope_id());
    }

    if (res != 0) {
        if (get_errno() == EINPROGRESS) {
            linux_event_watcher_t::watch_t watch(event_watcher.get(), poll_event_out);
            wait_interruptible(&watch, interruptor);
            int error;
            socklen_t error_size = sizeof(error);
            int getsockoptres = getsockopt(sock.get(), SOL_SOCKET, SO_ERROR, &error, &error_size);
            if (getsockoptres != 0) {
                throw linux_tcp_conn_t::connect_failed_exc_t(error);
            }
            if (error != 0) {
                throw linux_tcp_conn_t::connect_failed_exc_t(error);
            }
        } else {
            throw linux_tcp_conn_t::connect_failed_exc_t(get_errno());
        }
    }
}

linux_tcp_conn_t::linux_tcp_conn_t(fd_t s) :
        write_perfmon(nullptr),
        sock(s),
        event_watcher(new linux_event_watcher_t(sock.get(), this)),
        read_in_progress(false), write_in_progress(false),
        read_buffer(IO_BUFFER_SIZE),
        write_handler(this),
        write_queue_limiter(WRITE_QUEUE_MAX_SIZE),
        write_coro_pool(1, &write_queue, &write_handler),
        current_write_buffer(get_write_buffer()),
        drainer(new auto_drainer_t) {
    rassert(sock.get() != INVALID_FD);

    int res = fcntl(sock.get(), F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make socket non-blocking");
}

void linux_tcp_conn_t::enable_keepalive() {
    int optval = 1;
    int res = setsockopt(sock.get(), SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    guarantee(res != -1, "Could not set SO_KEEPALIVE option.");
}

linux_tcp_conn_t::write_buffer_t * linux_tcp_conn_t::get_write_buffer() {
    write_buffer_t *buffer;

    if (unused_write_buffers.empty()) {
        buffer = new write_buffer_t;
    } else {
        buffer = unused_write_buffers.head();
        unused_write_buffers.pop_front();
    }
    buffer->size = 0;
    return buffer;
}

linux_tcp_conn_t::write_queue_op_t * linux_tcp_conn_t::get_write_queue_op() {
    write_queue_op_t *op;

    if (unused_write_queue_ops.empty()) {
        op = new write_queue_op_t;
    } else {
        op = unused_write_queue_ops.head();
        unused_write_queue_ops.pop_front();
    }
    return op;
}

void linux_tcp_conn_t::release_write_buffer(write_buffer_t *buffer) {
    unused_write_buffers.push_front(buffer);
}

void linux_tcp_conn_t::release_write_queue_op(write_queue_op_t *op) {
    op->keepalive = auto_drainer_t::lock_t();
    unused_write_queue_ops.push_front(op);
}

size_t linux_tcp_conn_t::read_internal(void *buffer, size_t size) THROWS_ONLY(tcp_conn_read_closed_exc_t) {
    assert_thread();
    rassert(!read_closed.is_pulsed());

    while (true) {
        ssize_t res = ::read(sock.get(), buffer, size);

        if (res == -1 && (get_errno() == EAGAIN || get_errno() == EWOULDBLOCK)) {
            /* There's no data available right now, so we must wait for a notification from the
            epoll queue, or for an order to shut down. */

            linux_event_watcher_t::watch_t watch(event_watcher.get(), poll_event_in);
            wait_any_t waiter(&watch, &read_closed);
            waiter.wait_lazily_unordered();

            if (read_closed.is_pulsed()) {
                /* We were closed for whatever reason. Something else has already called
                on_shutdown_read(). In fact, we were probably signalled by on_shutdown_read(). */
                throw tcp_conn_read_closed_exc_t();
            }

            /* Go around the loop and try to read again */

        } else if (res == 0 || (res == -1 && (get_errno() == ECONNRESET || get_errno() == ENOTCONN))) {
            /* We were closed. This is the first notification that the kernel has given us, so we
            must call on_shutdown_read(). */
            on_shutdown_read();
            throw tcp_conn_read_closed_exc_t();

        } else if (res == -1) {
            /* Unknown error. This is not expected, but it will probably happen sometime so we
            shouldn't crash. */
            logERR("Could not read from socket: %s", errno_string(get_errno()).c_str());
            on_shutdown_read();
            throw tcp_conn_read_closed_exc_t();

        } else {
            /* We read some data, whooo */
            return res;
        }
    }
}

size_t linux_tcp_conn_t::read_some(void *buf, size_t size, signal_t *closer) THROWS_ONLY(tcp_conn_read_closed_exc_t) {
    rassert(size > 0);
    read_op_wrapper_t sentry(this, closer);

    if (read_buffer.size()) {
        /* Return the data from the peek buffer */
        size_t read_buffer_bytes = std::min(read_buffer.size(), size);
        memcpy(buf, read_buffer.data(), read_buffer_bytes);
        read_buffer.erase_front(read_buffer_bytes);
        return read_buffer_bytes;
    } else {
        /* Go to the kernel _once_. */
        return read_internal(buf, size);
    }
}

void linux_tcp_conn_t::read(void *buf, size_t size, signal_t *closer) THROWS_ONLY(tcp_conn_read_closed_exc_t) {
    read_op_wrapper_t sentry(this, closer);

    /* First, consume any data in the peek buffer */
    int read_buffer_bytes = std::min(read_buffer.size(), size);
    memcpy(buf, read_buffer.data(), read_buffer_bytes);
    read_buffer.erase_front(read_buffer_bytes);
    buf = reinterpret_cast<void *>(reinterpret_cast<char *>(buf) + read_buffer_bytes);
    size -= read_buffer_bytes;

    /* Now go to the kernel for any more data that we need */
    while (size > 0) {
        size_t delta = read_internal(buf, size);
        rassert(delta <= size);
        buf = reinterpret_cast<void *>(reinterpret_cast<char *>(buf) + delta);
        size -= delta;
    }
}

void linux_tcp_conn_t::read_buffered(void *buf, size_t size, signal_t *closer)
        THROWS_ONLY(tcp_conn_read_closed_exc_t) {
    while (size > 0) {
        const_charslice read_data = peek();
        if (read_data.end == read_data.beg) {
            // We didn't get anything from the read buffer. Get some data from
            // the underlying socket...
            // For large reads, we read directly into buf to avoid an additional copy
            // and additional round trips.
            // For smaller reads, we use `read_more_buffered` to read into the
            // connection's internal buffer and then copy out whatever we can use
            // to satisfy the current request.
            if (size >= IO_BUFFER_SIZE) {
                return read(buf, size, closer);
            } else {
                read_more_buffered(closer);
                read_data = peek();
            }
        }
        size_t num_read = read_data.end - read_data.beg;
        if (num_read > size) {
            num_read = size;
        }
        rassert(num_read > 0);
        memcpy(buf, read_data.beg, num_read);
        // Remove the consumed data from the read buffer
        pop(num_read, closer);

        size -= num_read;
        buf = static_cast<char *>(buf) + num_read;
    }
}

void linux_tcp_conn_t::read_more_buffered(signal_t *closer) THROWS_ONLY(tcp_conn_read_closed_exc_t) {
    read_op_wrapper_t sentry(this, closer);

    size_t old_size = read_buffer.size();
    read_buffer.resize(old_size + IO_BUFFER_SIZE);
    size_t delta = read_internal(read_buffer.data() + old_size, IO_BUFFER_SIZE);

    read_buffer.resize(old_size + delta);
}

const_charslice linux_tcp_conn_t::peek() const THROWS_ONLY(tcp_conn_read_closed_exc_t) {
    assert_thread();
    rassert(!read_in_progress);   // Is there a read already in progress?
    if (read_closed.is_pulsed()) {
        throw tcp_conn_read_closed_exc_t();
    }

    return const_charslice(read_buffer.data(), read_buffer.data() + read_buffer.size());
}

const_charslice linux_tcp_conn_t::peek(size_t size, signal_t *closer) THROWS_ONLY(tcp_conn_read_closed_exc_t) {
    while (read_buffer.size() < size) {
        read_more_buffered(closer);
    }
    return const_charslice(read_buffer.data(), read_buffer.data() + size);
}

void linux_tcp_conn_t::pop(size_t len, signal_t *closer) THROWS_ONLY(tcp_conn_read_closed_exc_t) {
    assert_thread();
    rassert(!read_in_progress);
    if (read_closed.is_pulsed()) {
        throw tcp_conn_read_closed_exc_t();
    }

    peek(len, closer);
    read_buffer.erase_front(len);
}

void linux_tcp_conn_t::shutdown_read() {
    assert_thread();
    int res = ::shutdown(sock.get(), SHUT_RD);
    if (res != 0 && get_errno() != ENOTCONN) {
        logERR("Could not shutdown socket for reading: %s", errno_string(get_errno()).c_str());
    }
    on_shutdown_read();
}

void linux_tcp_conn_t::on_shutdown_read() {
    assert_thread();
    rassert(!read_closed.is_pulsed());
    read_closed.pulse();
}

bool linux_tcp_conn_t::is_read_open() const {
    assert_thread();
    return !read_closed.is_pulsed();
}

linux_tcp_conn_t::write_handler_t::write_handler_t(linux_tcp_conn_t *_parent) :
    parent(_parent)
{ }

void linux_tcp_conn_t::write_handler_t::coro_pool_callback(write_queue_op_t *operation, UNUSED signal_t *interruptor) {
    if (operation->buffer != nullptr) {
        parent->perform_write(operation->buffer, operation->size);
        if (operation->dealloc != nullptr) {
            parent->release_write_buffer(operation->dealloc);
            parent->write_queue_limiter.unlock(operation->size);
        }
    }

    if (operation->cond != nullptr) {
        operation->cond->pulse();
    }
    if (operation->dealloc != nullptr) {
        parent->release_write_queue_op(operation);
    }
}

void linux_tcp_conn_t::internal_flush_write_buffer() {
    write_queue_op_t *op = get_write_queue_op();
    assert_thread();
    rassert(write_in_progress);

    /* Swap in a new write buffer, and set up the old write buffer to be
    released once the write is over. */
    op->buffer = current_write_buffer->buffer;
    op->size = current_write_buffer->size;
    op->dealloc = current_write_buffer.release();
    op->cond = nullptr;
    op->keepalive = auto_drainer_t::lock_t(drainer.get());
    current_write_buffer.init(get_write_buffer());

    /* Acquire the write semaphore so the write queue doesn't get too long
    to be released once the write is completed by the coroutine pool */
    rassert(op->size <= WRITE_CHUNK_SIZE);
    rassert(WRITE_CHUNK_SIZE < WRITE_QUEUE_MAX_SIZE);
    write_queue_limiter.co_lock(op->size);

    write_queue.push(op);
}

void linux_tcp_conn_t::perform_write(const void *buf, size_t size) {
    assert_thread();

    if (write_closed.is_pulsed()) {
        /* The write end of the connection was closed, but there are still
        operations in the write queue; we are one of those operations. Just
        don't do anything. */
        return;
    }

    while (size > 0) {
        ssize_t res = ::write(sock.get(), buf, size);

        if (res == -1 && (get_errno() == EAGAIN || get_errno() == EWOULDBLOCK)) {
            /* Wait for a notification from the event queue, or for an order to
            shut down */
            linux_event_watcher_t::watch_t watch(event_watcher.get(), poll_event_out);
            wait_any_t waiter(&watch, &write_closed);
            waiter.wait_lazily_unordered();

            if (write_closed.is_pulsed()) {
                /* We were closed for whatever reason. Whatever signalled us has already called
                   on_shutdown_write(). */
                break;
            }

            /* Go around the loop and try to write again */

        } else if (res == -1 && (get_errno() == EPIPE || get_errno() == ENOTCONN || get_errno() == EHOSTUNREACH ||
                                 get_errno() == ENETDOWN || get_errno() == EHOSTDOWN || get_errno() == ECONNRESET)) {
            /* These errors are expected to happen at some point in practice */
            on_shutdown_write();
            break;

        } else if (res == -1) {
            /* In theory this should never happen, but it probably will. So we write a log message
               and then shut down normally. */
            logERR("Could not write to socket: %s", errno_string(get_errno()).c_str());
            on_shutdown_write();
            break;

        } else if (res == 0) {
            /* This should never happen either, but it's better to write an error message than to
               crash completely. */
            logERR("Didn't expect write() to return 0.");
            on_shutdown_write();
            break;

        } else {
            rassert(res <= static_cast<ssize_t>(size));
            buf = reinterpret_cast<const void *>(reinterpret_cast<const char *>(buf) + res);
            size -= res;
            if (write_perfmon) {
                write_perfmon->record(res);
            }
        }
    }
}

void linux_tcp_conn_t::write(const void *buf, size_t size, signal_t *closer) THROWS_ONLY(tcp_conn_write_closed_exc_t) {
    write_op_wrapper_t sentry(this, closer);

    write_queue_op_t op;
    cond_t to_signal_when_done;

    /* Flush out any data that's been buffered, so that things don't get out of order */
    if (current_write_buffer->size > 0) {
        internal_flush_write_buffer();
    }

    /* Don't bother acquiring the write semaphore because we're going to block
    until the write is done anyway */

    /* Enqueue the write so it will happen eventually */
    op.buffer = buf;
    op.size = size;
    op.dealloc = nullptr;
    op.cond = &to_signal_when_done;
    write_queue.push(&op);

    /* Wait for the write to be done. If the write half of the network connection
    is closed before or during our write, then `perform_write()` will turn into a
    no-op, so the cond will still get pulsed. */
    to_signal_when_done.wait();

    if (write_closed.is_pulsed()) {
        throw tcp_conn_write_closed_exc_t();
    }
}

void linux_tcp_conn_t::write_buffered(const void *vbuf, size_t size, signal_t *closer) THROWS_ONLY(tcp_conn_write_closed_exc_t) {
    write_op_wrapper_t sentry(this, closer);

    /* Convert to `char` for ease of pointer arithmetic */
    const char *buf = reinterpret_cast<const char *>(vbuf);

    while (size > 0) {
        /* Stop putting more things on the write queue if it's already closed. */
        if (write_closed.is_pulsed()) {
            throw tcp_conn_write_closed_exc_t();
        }

        /* Insert the largest chunk that fits in this block */
        size_t chunk = std::min(size, WRITE_CHUNK_SIZE - current_write_buffer->size);

        memcpy(current_write_buffer->buffer + current_write_buffer->size, buf, chunk);
        current_write_buffer->size += chunk;

        rassert(current_write_buffer->size <= WRITE_CHUNK_SIZE);
        if (current_write_buffer->size == WRITE_CHUNK_SIZE) {
            internal_flush_write_buffer();
        }

        buf += chunk;
        size -= chunk;
    }

    if (write_closed.is_pulsed()) {
        throw tcp_conn_write_closed_exc_t();
    }
}

void linux_tcp_conn_t::writef(signal_t *closer, const char *format, ...) THROWS_ONLY(tcp_conn_write_closed_exc_t) {
    va_list ap;
    va_start(ap, format);

    printf_buffer_t b(ap, format);
    write(b.data(), b.size(), closer);

    va_end(ap);
}

void linux_tcp_conn_t::flush_buffer(signal_t *closer) THROWS_ONLY(tcp_conn_write_closed_exc_t) {
    write_op_wrapper_t sentry(this, closer);

    /* Flush the write buffer; it might be half-full. */
    if (current_write_buffer->size > 0) {
        internal_flush_write_buffer();
    }

    /* Wait until we know that the write buffer has gone out over the network.
    If the write half of the connection is closed, then the call to
    `perform_write()` that `internal_flush_write_buffer()` will turn into a no-op,
    but the queue will continue to be pumped and so our cond will still get
    pulsed. */
    write_queue_op_t op;
    cond_t to_signal_when_done;
    op.buffer = nullptr;
    op.dealloc = nullptr;
    op.cond = &to_signal_when_done;
    write_queue.push(&op);
    to_signal_when_done.wait();

    if (write_closed.is_pulsed()) {
        throw tcp_conn_write_closed_exc_t();
    }
}

void linux_tcp_conn_t::flush_buffer_eventually(signal_t *closer) THROWS_ONLY(tcp_conn_write_closed_exc_t) {
    write_op_wrapper_t sentry(this, closer);

    /* Flush the write buffer; it might be half-full. */
    if (current_write_buffer->size > 0) {
        internal_flush_write_buffer();
    }

    if (write_closed.is_pulsed()) {
        throw tcp_conn_write_closed_exc_t();
    }
}

void linux_tcp_conn_t::shutdown_write() {
    assert_thread();

    int res = ::shutdown(sock.get(), SHUT_WR);
    if (res != 0 && get_errno() != ENOTCONN) {
        logERR("Could not shutdown socket for writing: %s", errno_string(get_errno()).c_str());
    }

    on_shutdown_write();
}

void linux_tcp_conn_t::on_shutdown_write() {
    assert_thread();
    rassert(!write_closed.is_pulsed());
    write_closed.pulse();

    /* We don't flush out the write queue or stop the write coro pool explicitly.
    But by pulsing `write_closed`, we turn all `perform_write()` operations into
    no-ops, so in practice the write queue empties. */
}

bool linux_tcp_conn_t::is_write_open() const {
    assert_thread();
    return !write_closed.is_pulsed();
}

linux_tcp_conn_t::~linux_tcp_conn_t() THROWS_NOTHING {
    assert_thread();

    // Tell the readers and writers to stop.  The auto drainer will
    // wait for them to stop.
    if (is_read_open()) {
        shutdown_read();
    }
    if (is_write_open()) {
        shutdown_write();
    }
}

void linux_tcp_conn_t::rethread(threadnum_t new_thread) {
    if (home_thread() == get_thread_id() && new_thread == INVALID_THREAD) {
        rassert(!read_in_progress);
        rassert(!write_in_progress);
        rassert(event_watcher.has());
        event_watcher.reset();

    } else if (home_thread() == INVALID_THREAD && new_thread == get_thread_id()) {
        rassert(!event_watcher.has());
        event_watcher.init(new linux_event_watcher_t(sock.get(), this));

    } else {
        crash("linux_tcp_conn_t can be rethread()ed from no thread to the current thread or "
              "from the current thread to no thread, but no other combination is legal. The "
              "current thread is %" PRIi32 "; the old thread is %" PRIi32 "; the new thread "
              "is %" PRIi32 ".\n",
              get_thread_id().threadnum, home_thread().threadnum, new_thread.threadnum);
    }

    real_home_thread = new_thread;

    read_closed.rethread(new_thread);
    write_closed.rethread(new_thread);
    write_coro_pool.rethread(new_thread);

    if (drainer.has()) {
        drainer->rethread(new_thread);
    }
}

bool linux_tcp_conn_t::getpeername(ip_and_port_t *ip_and_port) {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    int res = ::getpeername(sock.get(), reinterpret_cast<sockaddr *>(&addr), &addr_len);
    if (res == 0) {
        *ip_and_port = ip_and_port_t(reinterpret_cast<sockaddr *>(&addr));
        return true;
    }

    return false;
}

void linux_tcp_conn_t::on_event(int /* events */) {
    assert_thread();

    /* This is called by linux_event_watcher_t when error events occur. Ordinary
    poll_event_in/poll_event_out events are not sent through this function. */

    if (is_write_open()) {
        shutdown_write();
    }

    if (is_read_open()) {
        shutdown_read();
    }

    event_watcher->stop_watching_for_errors();
}

tls_conn_wrapper_t::tls_conn_wrapper_t(SSL_CTX *tls_ctx)
    THROWS_ONLY(linux_tcp_conn_t::connect_failed_exc_t) {
    ERR_clear_error();

    conn = SSL_new(tls_ctx);
    if (nullptr == conn) {
        unsigned long err_code = ERR_get_error(); // NOLINT(runtime/int)

        throw linux_tcp_conn_t::connect_failed_exc_t(
            err_code, ERR_error_string(err_code, nullptr));
    }

    // Add support for partial writes.
    SSL_set_mode(conn, SSL_MODE_ENABLE_PARTIAL_WRITE);
}

tls_conn_wrapper_t::~tls_conn_wrapper_t() {
    SSL_free(conn);
}

// Set the underlying IO.
void tls_conn_wrapper_t::set_fd(fd_t sock)
    THROWS_ONLY(linux_tcp_conn_t::connect_failed_exc_t) {
    if (0 == SSL_set_fd(conn, sock)) {
        unsigned long err_code = ERR_get_error(); // NOLINT(runtime/int)
        throw linux_tcp_conn_t::connect_failed_exc_t(
            err_code, ERR_error_string(err_code, nullptr));
    }
}

/* This is the client version of the constructor. The base class constructor
will establish a TCP connection to the peer at the given host:port and then we
wrap the tcp connection in TLS using the configuration in the given tls_ctx. */
linux_secure_tcp_conn_t::linux_secure_tcp_conn_t(
    SSL_CTX *tls_ctx, const ip_address_t &host, int port,
    signal_t *interruptor, int local_port) THROWS_ONLY(connect_failed_exc_t, interrupted_exc_t) :
    linux_tcp_conn_t(host, port, interruptor, local_port),
    conn(tls_ctx) {

    conn.set_fd(sock.get());
    SSL_set_connect_state(conn.get());
    perform_handshake(interruptor);
}

/* This is the server version of the constructor */
linux_secure_tcp_conn_t::linux_secure_tcp_conn_t(
    SSL_CTX *tls_ctx, fd_t _sock, signal_t *interruptor)
    THROWS_ONLY(connect_failed_exc_t, interrupted_exc_t) :
    linux_tcp_conn_t(_sock),
    conn(tls_ctx) {

    conn.set_fd(sock.get());
    SSL_set_accept_state(conn.get());
    perform_handshake(interruptor);
}

linux_secure_tcp_conn_t::~linux_secure_tcp_conn_t() THROWS_NOTHING {
    assert_thread();

    if (is_open()) shutdown();
}

void linux_secure_tcp_conn_t::rethread(threadnum_t thread) {
    closed.rethread(thread);

    linux_tcp_conn_t::rethread(thread);
}

void linux_secure_tcp_conn_t::perform_handshake(signal_t *interruptor)
    THROWS_ONLY(linux_tcp_conn_t::connect_failed_exc_t, interrupted_exc_t) {
    // Perform TLS handshake.
    while (true) {
        ERR_clear_error();
        int ret = SSL_do_handshake(conn.get());

        if (ret > 0) {
            return; // Successful TLS handshake.
        }

        if (ret == 0) {
            // The handshake failed but the connection shut down cleanly.
            throw linux_tcp_conn_t::connect_failed_exc_t(
                0, "TLS handshake failed, shutdown cleanly");
        }

        switch (SSL_get_error(conn.get(), ret)) {
        case SSL_ERROR_WANT_READ:
            /* The handshake needs to read data, but the underlying I/O has no data
            ready to read. Wait for it to be ready or for an interrupt signal. */
            {
                linux_event_watcher_t::watch_t watch(get_event_watcher(), poll_event_in);
                wait_interruptible(&watch, interruptor);
            }
            break;
        case SSL_ERROR_WANT_WRITE:
            /* The handshake needs to write data, but the underlying I/O is not ready
            to write. Wait for it to be ready or for an interrupt signal. */
            {
                linux_event_watcher_t::watch_t watch(get_event_watcher(), poll_event_out);
                wait_interruptible(&watch, interruptor);
            }
            break;
        default:
            // Some other error with the underlying I/O.
            unsigned long err_code = ERR_get_error(); // NOLINT(runtime/int)
            throw linux_tcp_conn_t::connect_failed_exc_t(
                err_code, ERR_error_string(err_code, nullptr));
        }

        if (interruptor->is_pulsed()) {
            // The handshake cannot continue because we need to shutdown now.
            throw interrupted_exc_t();
        }

        /* Go around the loop and try to complete the handshake. */
    }
}

size_t linux_secure_tcp_conn_t::read_internal(void *buffer, size_t size)
    THROWS_ONLY(tcp_conn_read_closed_exc_t) {
    assert_thread();
    rassert(!closed.is_pulsed());

    while(true) {
        ERR_clear_error();

        int ret = SSL_read(conn.get(), buffer, size);

        if (ret > 0) {
            return ret; // Operation successful, returns number of bytes read.
        }

        switch (SSL_get_error(conn.get(), ret)) {
        case SSL_ERROR_ZERO_RETURN:
            // Indicates that the peer has sent the "close notify" alert. The
            // shutdown state is currently SSL_RECEIVED_SHUTDOWN. We must now
            // send our "close notify" alert.
            shutdown();
            throw tcp_conn_read_closed_exc_t();
        case SSL_ERROR_WANT_READ:
            /* The underlying I/O has no data ready to read. Wait for it to be
            ready or for someone to send a close signal. */
            {
                linux_event_watcher_t::watch_t watch(
                    get_event_watcher(), poll_event_in);
                wait_any_t waiter(&watch, &closed);
                waiter.wait_lazily_unordered();
            }
            break;
        case SSL_ERROR_WANT_WRITE:
            /* Though we are reading, a TLS renegotiation may occur at any time
            requiring a write. Wait for the underyling I/O to be ready for a
            write, or for someone to send a close signal. */
            {
                linux_event_watcher_t::watch_t watch(
                    get_event_watcher(), poll_event_out);
                wait_any_t waiter(&watch, &closed);
                waiter.wait_lazily_unordered();
            }
            break;
        default:
            // Some other error. Assume that the connection is unusable.
            shutdown_socket();
            throw tcp_conn_read_closed_exc_t();
        }

        if (closed.is_pulsed()) {
            /* We were closed for whatever reason. Whatever signalled us has
            already called shutdown_socket(). */
            throw tcp_conn_read_closed_exc_t();
        }

        /* Go around the loop and try to read again */
    }
}

void linux_secure_tcp_conn_t::perform_write(const void *buffer, size_t size) {
    assert_thread();

    if (closed.is_pulsed()) {
        /* The connection was closed, but there are still operations in the
        write queue; we are one of those operations. Just don't do anything. */
        return;
    }

    // Loop for retrying if the underlying socket would block and to retry on
    // partial writes.
    while (size > 0) {
        ERR_clear_error();

        int ret = SSL_write(conn.get(), buffer, size);

        if (ret > 0) {
            // Operation successful, returns number of bytes written.
            rassert(static_cast<size_t>(ret) <= size);
            size -= ret;

            // Slide down the buffer.
            buffer = reinterpret_cast<const void *>(
                reinterpret_cast<const char *>(buffer) + ret);

            if (write_perfmon) write_perfmon->record(ret);

            // Go around the loop again if there is more data to write.
            continue;
        }

        switch (SSL_get_error(conn.get(), ret)) {
        case SSL_ERROR_ZERO_RETURN:
            // Indicates that the peer has sent the "close notify" alert. The
            // shutdown state is currently SSL_RECEIVED_SHUTDOWN. We must now
            // send our "close notify" alert.
            shutdown();
            return;
        case SSL_ERROR_WANT_READ:
            /* Though we are writing, a TLS renegotiation may occur at any time
            requiring a read. Wait for the underyling I/O to be ready for a
            read, or for someone to send a close signal. */
            {
                linux_event_watcher_t::watch_t watch(get_event_watcher(), poll_event_in);
                wait_any_t waiter(&watch, &closed);
                waiter.wait_lazily_unordered();
            }
            break;
        case SSL_ERROR_WANT_WRITE:
            /* The underlying I/O is not ready to accept a write. Wait for it
            to be ready or for someone to send a close signal. */
            {
                linux_event_watcher_t::watch_t watch(
                    get_event_watcher(), poll_event_out);
                wait_any_t waiter(&watch, &closed);
                waiter.wait_lazily_unordered();
            }
            break;
        default:
            // Some other error. Assume that the connection is unusable.
            shutdown_socket();
            return;
        }

        if (closed.is_pulsed()) {
            /* We were closed for whatever reason. Whatever signalled
            us has already called shutdown_socket(). */
            throw tcp_conn_read_closed_exc_t();
        }

        /* Go around the loop and try to read again */
    }
}

/* It is not possible to close only the read or write side of a TLS connection
so we use only a single shutdown method which attempts to shutdown the TLS
before shutting down the underlying tcp connection */
void linux_secure_tcp_conn_t::shutdown() {
    assert_thread();

    // If something else already shut us down, abort immediately.
    if (closed.is_pulsed()) {
        return;
    }

    // Wait at most 5 seconds for the orderly shutdown. If it doesn't complete by then,
    // we simply shutdown the socket.
    signal_timer_t shutdown_timeout(5000);

    bool skip_shutdown = false; // Set to true if TLS shutdown encounters an error.

    // If we have not already received a "close notify" alert from the peer,
    // then we should send one. If we have received one, this loop will respond
    // with our own "close notify" alert.
    while (!(skip_shutdown || shutdown_timeout.is_pulsed())) {
        ERR_clear_error();

        int ret = SSL_shutdown(conn.get());

        if (ret > 0) {
            // "close notify" has been sent and received.
            break;
        }

        if (ret == 0) {
            // "close notify" has been sent but not yet received from peer.
            continue;
        }

        switch(SSL_get_error(conn.get(), ret)) {
        case SSL_ERROR_WANT_READ:
            {
                /* The shutdown needs to read data, but the underlying I/O has no data
                ready to read. Wait for it to be ready or for a timeout. */
                linux_event_watcher_t::watch_t watch(get_event_watcher(), poll_event_in);
                wait_any_t waiter(&watch, &shutdown_timeout);
                waiter.wait_lazily_unordered();
            }
            continue;
        case SSL_ERROR_WANT_WRITE:
            {
                /* The handshake needs to write data, but the underlying I/O is not ready
                to write. Wait for it to be ready or for a timeout. */
                linux_event_watcher_t::watch_t watch(
                    get_event_watcher(), poll_event_out);
                wait_any_t waiter(&watch, &shutdown_timeout);
                waiter.wait_lazily_unordered();
            }
            continue;
        default:
            // Unable to perform clean shutdown. Just skip it.
            skip_shutdown = true;
        }
    }

    shutdown_socket();
}

void linux_secure_tcp_conn_t::shutdown_socket() {
    assert_thread();
    rassert(!closed.is_pulsed());
    rassert(!read_closed.is_pulsed());
    rassert(!write_closed.is_pulsed());

    // Shutdown the underlying TCP connection.
    int res = ::shutdown(sock.get(), SHUT_RDWR);
    if (res != 0 && get_errno() != ENOTCONN) {
        logERR(
            "Could not shutdown socket for reading and writing: %s",
            errno_string(get_errno()).c_str());
    }

    closed.pulse();
    read_closed.pulse();
    write_closed.pulse();
}


linux_tcp_conn_descriptor_t::linux_tcp_conn_descriptor_t(fd_t fd) : fd_(fd) {
    rassert(fd != -1);
}

linux_tcp_conn_descriptor_t::~linux_tcp_conn_descriptor_t() {
    if (fd_ != -1) {
        int res = ::shutdown(fd_, SHUT_RDWR);
        if (res != 0 && get_errno() != ENOTCONN) {
            logERR(
                "Could not shutdown socket for reading and writing: %s",
                errno_string(get_errno()).c_str());
        }
    }
}

void linux_tcp_conn_descriptor_t::make_server_connection(
    SSL_CTX *tls_ctx, scoped_ptr_t<linux_tcp_conn_t> *tcp_conn, signal_t *closer
) THROWS_ONLY(linux_tcp_conn_t::connect_failed_exc_t, interrupted_exc_t) {
    // We pass ownership of `fd_` to the connection.
    fd_t sock = fd_;
    fd_ = -1;
    if (tls_ctx == nullptr) {
        tcp_conn->init(new linux_tcp_conn_t(sock));
    } else {
        tcp_conn->init(new linux_secure_tcp_conn_t(tls_ctx, sock, closer));
    }
}

void linux_tcp_conn_descriptor_t::make_server_connection(
    SSL_CTX *tls_ctx, linux_tcp_conn_t **tcp_conn_out, signal_t *closer
) THROWS_ONLY(linux_tcp_conn_t::connect_failed_exc_t, interrupted_exc_t) {
    // We pass ownership of `fd_` to the connection.
    fd_t sock = fd_;
    fd_ = -1;
    if (tls_ctx == nullptr) {
        *tcp_conn_out = new linux_tcp_conn_t(sock);
    } else {
        *tcp_conn_out = new linux_secure_tcp_conn_t(tls_ctx, sock, closer);
    }
}

/* Network listener object */
linux_nonthrowing_tcp_listener_t::linux_nonthrowing_tcp_listener_t(
        const std::set<ip_address_t> &bind_addresses, int _port,
        const std::function<void(scoped_ptr_t<linux_tcp_conn_descriptor_t> &)> &cb) :
    callback(cb),
    local_addresses(bind_addresses),
    port(_port),
    bound(false),
    socks(),
    last_used_socket_index(0),
    event_watchers(),
    log_next_error(true)
{
    // If no addresses were supplied, default to 'any'
    if (local_addresses.empty()) {
        local_addresses.insert(ip_address_t::any(AF_INET6));
    }
}

bool linux_nonthrowing_tcp_listener_t::begin_listening() {
    if (!bound) {
        try {
            bind_sockets();
        } catch (const tcp_socket_exc_t &) {
            return false;
        }
    }

    const int RDB_LISTEN_BACKLOG = 256;

    // Start listening to connections
    for (size_t i = 0; i < socks.size(); ++i) {
        int res = listen(socks[i].get(), RDB_LISTEN_BACKLOG);
        guarantee_err(res == 0, "Couldn't listen to the socket");

        res = fcntl(socks[i].get(), F_SETFL, O_NONBLOCK);
        guarantee_err(res == 0, "Could not make socket non-blocking");
    }

    // Start the accept loop
    accept_loop_drainer.init(new auto_drainer_t);
    coro_t::spawn_sometime(std::bind(
        &linux_nonthrowing_tcp_listener_t::accept_loop, this, auto_drainer_t::lock_t(accept_loop_drainer.get())));

    return true;
}

bool linux_nonthrowing_tcp_listener_t::is_bound() const {
    return bound;
}

int linux_nonthrowing_tcp_listener_t::get_port() const {
    return port;
}

int linux_nonthrowing_tcp_listener_t::init_sockets() {
    event_watchers.reset();
    event_watchers.init(local_addresses.size());
    socks.reset();
    socks.init(local_addresses.size());

    size_t i = 0;
    for (auto addr = local_addresses.begin(); addr != local_addresses.end(); ++addr, ++i) {
        if (event_watchers[i].has()) {
            event_watchers[i].reset();
        }

        socks[i].reset(socket(addr->get_address_family(), SOCK_STREAM, 0));
        if (socks[i].get() == INVALID_FD) {
            return get_errno();
        }

        event_watchers[i].init(new linux_event_watcher_t(socks[i].get(), this));

        int sock_fd = socks[i].get();
        guarantee_err(sock_fd != INVALID_FD, "Couldn't create socket");

        int sockoptval = 1;
        int res = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &sockoptval, sizeof(sockoptval));
        guarantee_err(res != -1, "Could not set REUSEADDR option");

        /* XXX Making our socket NODELAY prevents the problem where responses to
         * pipelined requests are delayed, since the TCP Nagle algorithm will
         * notice when we send multiple small packets and try to coalesce them. But
         * if we are only sending a few of these small packets quickly, like during
         * pipeline request responses, then Nagle delays for around 40 ms before
         * sending out those coalesced packets if they don't reach the max window
         * size. So for latency's sake we want to disable Nagle.
         *
         * This might decrease our throughput, so perhaps we should add a
         * runtime option for it.
         */
        res = setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &sockoptval, sizeof(sockoptval));
        guarantee_err(res != -1, "Could not set TCP_NODELAY option");
    }
    return 0;
}

int bind_ipv4_interface(fd_t sock, int *port_out, const struct in_addr &addr) {
    sockaddr_in serv_addr;
    socklen_t sa_len(sizeof(serv_addr));
    memset(&serv_addr, 0, sa_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(*port_out);
    serv_addr.sin_addr = addr;

    int res = bind(sock, reinterpret_cast<sockaddr *>(&serv_addr), sa_len);
    if (res != 0) {
        res = get_errno();
    } else if (*port_out == ANY_PORT) {
        res = ::getsockname(sock, reinterpret_cast<sockaddr *>(&serv_addr), &sa_len);
        guarantee_err(res != -1, "Could not determine socket local port number");
        *port_out = ntohs(serv_addr.sin_port);
    }

    return res;
}

int bind_ipv6_interface(fd_t sock, int *port_out, const ip_address_t &addr) {
    sockaddr_in6 serv_addr;
    socklen_t sa_len(sizeof(serv_addr));
    memset(&serv_addr, 0, sa_len);
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(*port_out);
    serv_addr.sin6_addr = addr.get_ipv6_addr();
    serv_addr.sin6_scope_id = addr.get_ipv6_scope_id();

    int res = bind(sock, reinterpret_cast<sockaddr *>(&serv_addr), sa_len);
    if (res != 0) {
        res = get_errno();
    } else if (*port_out == ANY_PORT) {
        res = ::getsockname(sock, reinterpret_cast<sockaddr *>(&serv_addr), &sa_len);
        guarantee_err(res != -1, "Could not determine socket local port number");
        *port_out = ntohs(serv_addr.sin6_port);
    }

    return res;
}

void fallback_to_ipv4(std::set<ip_address_t> *addrs, int err, int port) {
    bool ipv6_address_found = false;

    // Fallback to IPv4 only - remove any IPv6 addresses and resize dependant arrays
    for (auto it = addrs->begin(); it != addrs->end();) {
        if (it->get_address_family() == AF_INET6) {
            if (it->is_any()) {
                addrs->insert(ip_address_t::any(AF_INET));
            }
            addrs->erase(*(it++));
            ipv6_address_found = true;
        } else {
            ++it;
        }
    }

    if (!ipv6_address_found) {
        throw tcp_socket_exc_t(err, port);
    }
    logERR("Failed to create sockets for listener on port %d, "
           "falling back to IPv4 only", port);
}

void linux_nonthrowing_tcp_listener_t::bind_sockets() {
    // It may take multiple attempts to get all the sockets onto the same port
    int local_port = port;

    for (uint32_t attempts = (port == ANY_PORT) ? MAX_BIND_ATTEMPTS : 1;
         attempts > 0; --attempts) {
        local_port = port;
        int res = init_sockets();
        if (res == EAFNOSUPPORT) {
            ++attempts; // This attempt doesn't count
            fallback_to_ipv4(&local_addresses, res, port);
            continue;
        } else if (res != 0) {
            throw tcp_socket_exc_t(res, port);
        }

        size_t i = 0;
        for (std::set<ip_address_t>::iterator addr = local_addresses.begin();
             addr != local_addresses.end(); ++i, ++addr) {
            switch (addr->get_address_family()) {
            case AF_INET:
                res = bind_ipv4_interface(socks[i].get(), &local_port, addr->get_ipv4_addr());
                break;
            case AF_INET6:
                res = bind_ipv6_interface(socks[i].get(), &local_port, *addr);
                break;
            default:
                unreachable();
            }

            if (res == EADDRNOTAVAIL) {
                ++attempts; // This attempt doesn't count
                fallback_to_ipv4(&local_addresses, res, port);
                break;
            } else if (res == EADDRINUSE || res == EACCES) {
                break;
            } else if (res != 0) {
                throw tcp_socket_exc_t(res, port);
            }
        }

        if (i == local_addresses.size()) {
            bound = true;
            port = local_port;
            return;
        }
    }
    throw tcp_socket_exc_t(EADDRINUSE, port);
}

fd_t linux_nonthrowing_tcp_listener_t::wait_for_any_socket(const auto_drainer_t::lock_t &lock) {
    scoped_array_t<scoped_ptr_t<linux_event_watcher_t::watch_t> > watches(event_watchers.size());
    wait_any_t waiter(lock.get_drain_signal());

    for (size_t i = 0; i < event_watchers.size(); ++i) {
        watches[i].init(new linux_event_watcher_t::watch_t(event_watchers[i].get(), poll_event_in));
        waiter.add(watches[i].get());
    }

    waiter.wait_lazily_unordered();

    if (lock.get_drain_signal()->is_pulsed()) {
        return -1;
    }

    for (size_t i = 0; i < watches.size(); ++i) {
        // This rather convoluted expression is to make sure we don't starve out higher-indexed interfaces
        //  because connections are coming in too fast on the lower interfaces, unlikely but valid
        size_t index = (last_used_socket_index + i + 1) % watches.size();
        if (watches[index]->is_pulsed()) {
            last_used_socket_index = index;
            return socks[index].get();
        }
    }

    // This should never happen, but it shouldn't be much of a problem
    return -1;
}

void linux_nonthrowing_tcp_listener_t::accept_loop(auto_drainer_t::lock_t lock) {
    exponential_backoff_t backoff(10, 160, 2.0, 0.5);
    fd_t active_fd = socks[0].get();

    while(!lock.get_drain_signal()->is_pulsed()) {
        fd_t new_sock = accept(active_fd, nullptr, nullptr);

        if (new_sock != INVALID_FD) {
            coro_t::spawn_now_dangerously(std::bind(&linux_nonthrowing_tcp_listener_t::handle, this, new_sock));
            backoff.success();

            /* Assume that if there was a problem before, it's gone now because accept()
            is working. */
            log_next_error = true;

        } else if (get_errno() == EAGAIN || get_errno() == EWOULDBLOCK) {
            active_fd = wait_for_any_socket(lock);

        } else if (get_errno() == EINTR) {
            /* Harmless error; just try again. */

        } else {
            /* Unexpected error. Log it unless it's a repeat error. */
            if (log_next_error) {
                logERR("accept() failed: %s.",
                    errno_string(get_errno()).c_str());
                log_next_error = false;
            }

            /* Delay before retrying. */
            try {
                backoff.failure(lock.get_drain_signal());
            } catch (const interrupted_exc_t &) {
                return;
            }
        }
    }
}

void linux_nonthrowing_tcp_listener_t::handle(fd_t socket) {
    scoped_ptr_t<linux_tcp_conn_descriptor_t> nconn(new linux_tcp_conn_descriptor_t(socket));
    callback(nconn);
}

linux_nonthrowing_tcp_listener_t::~linux_nonthrowing_tcp_listener_t() {
    /* Interrupt the accept loop */
    accept_loop_drainer.reset();

    // scoped_fd_t destructor will close() the socket
}

void linux_nonthrowing_tcp_listener_t::on_event(int) {
    /* This is only called in cases of error; normal input events are recieved
    via event_listener.watch(). */
}

void noop_fun(UNUSED const scoped_ptr_t<linux_tcp_conn_descriptor_t> &arg) { }

linux_tcp_bound_socket_t::linux_tcp_bound_socket_t(const std::set<ip_address_t> &bind_addresses, int port) :
    listener(new linux_nonthrowing_tcp_listener_t(bind_addresses, port, noop_fun))
{
    listener->bind_sockets();
}

int linux_tcp_bound_socket_t::get_port() const {
    return listener->get_port();
}

linux_tcp_listener_t::linux_tcp_listener_t(const std::set<ip_address_t> &bind_addresses, int port,
    const std::function<void(scoped_ptr_t<linux_tcp_conn_descriptor_t> &)> &callback) :
        listener(new linux_nonthrowing_tcp_listener_t(bind_addresses, port, callback))
{
    if (!listener->begin_listening()) {
        throw address_in_use_exc_t("localhost", listener->get_port());
    }
}

linux_tcp_listener_t::linux_tcp_listener_t(
    linux_tcp_bound_socket_t *bound_socket,
    const std::function<void(scoped_ptr_t<linux_tcp_conn_descriptor_t> &)> &callback) :
        listener(bound_socket->listener.release())
{
    listener->callback = callback;
    if (!listener->begin_listening()) {
        throw address_in_use_exc_t("localhost", listener->get_port());
    }
}

int linux_tcp_listener_t::get_port() const {
    return listener->get_port();
}

linux_repeated_nonthrowing_tcp_listener_t::linux_repeated_nonthrowing_tcp_listener_t(
    const std::set<ip_address_t> &bind_addresses,
    int port,
    const std::function<void(scoped_ptr_t<linux_tcp_conn_descriptor_t> &)> &callback) :
        listener(bind_addresses, port, callback)
{ }

int linux_repeated_nonthrowing_tcp_listener_t::get_port() const {
    return listener.get_port();
}

void linux_repeated_nonthrowing_tcp_listener_t::begin_repeated_listening_attempts() {
    auto_drainer_t::lock_t lock(&drainer);
    coro_t::spawn_sometime(
        std::bind(&linux_repeated_nonthrowing_tcp_listener_t::retry_loop, this, lock));
}

void linux_repeated_nonthrowing_tcp_listener_t::retry_loop(auto_drainer_t::lock_t lock) {
    try {
        bool bound = listener.begin_listening();

        for (int retry_interval = 1;
             !bound;
             retry_interval = std::min(10, retry_interval + 2)) {
            logNTC("Will retry binding to port %d in %d seconds.\n",
                    listener.get_port(),
                    retry_interval);
            nap(retry_interval * 1000, lock.get_drain_signal());
            bound = listener.begin_listening();
        }

        bound_cond.pulse();
    } catch (const interrupted_exc_t &e) {
        // ignore
    }
}

signal_t *linux_repeated_nonthrowing_tcp_listener_t::get_bound_signal() {
    return &bound_cond;
}

std::vector<std::string> get_ips() {
    std::vector<std::string> ret;

    struct ifaddrs *if_addrs = nullptr;
    int addr_res = getifaddrs(&if_addrs);
    guarantee_err(addr_res == 0, "getifaddrs failed, could not determine local ip addresses");

    for (ifaddrs *p = if_addrs; p != nullptr; p = p->ifa_next) {
        if (p->ifa_addr == nullptr) {
            continue;
        } else if (p->ifa_addr->sa_family == AF_INET) {
            if (!(p->ifa_flags & IFF_LOOPBACK)) {
                struct sockaddr_in *in_addr = reinterpret_cast<sockaddr_in *>(p->ifa_addr);
                // I don't think the "+ 1" is necessary, we're being
                // paranoid about weak documentation.
                const int buflength = INET_ADDRSTRLEN;
                char buf[buflength + 1] = { 0 };
                const char *res = inet_ntop(AF_INET, &in_addr->sin_addr, buf, buflength);

                guarantee_err(res != nullptr, "inet_ntop failed");

                ret.push_back(std::string(buf));
            }
        } else if (p->ifa_addr->sa_family == AF_INET6) {
            if (!(p->ifa_flags & IFF_LOOPBACK)) {
                struct sockaddr_in6 *in6_addr = reinterpret_cast<sockaddr_in6 *>(p->ifa_addr);

                const int buflength = INET6_ADDRSTRLEN;
                scoped_array_t<char> buf(buflength + 1);
                memset(buf.data(), 0, buf.size());
                const char *res = inet_ntop(AF_INET6, &in6_addr->sin6_addr, buf.data(), buflength);

                guarantee_err(res != nullptr, "inet_ntop failed on an ipv6 address");

                ret.push_back(std::string(buf.data()));
            }
        }
    }

    freeifaddrs(if_addrs);

    return ret;
}

#endif
