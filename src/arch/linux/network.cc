#include "arch/linux/network.hpp"
#include "arch/linux/thread_pool.hpp"
#include "arch/timing.hpp"
#include "logger.hpp"
#include "concurrency/cond_var.hpp"
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/sockios.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iterator>
#include <algorithm>

const uint32_t linux_buffered_tcp_conn_t::READ_BUFFER_RESIZE_FACTOR = 2;
const uint32_t linux_buffered_tcp_conn_t::READ_BUFFER_MIN_UTILIZATION_FACTOR = 4; // if less than 1/4 of the buffer is used, shrink it
const size_t linux_buffered_tcp_conn_t::MIN_BUFFERED_READ_SIZE = 1024;
const size_t linux_buffered_tcp_conn_t::MIN_READ_BUFFER_SIZE = linux_buffered_tcp_conn_t::MIN_BUFFERED_READ_SIZE * 4;
const size_t linux_buffered_tcp_conn_t::MAX_WRITE_BUFFER_SIZE = 8192;
const size_t linux_raw_tcp_conn_t::ZEROCOPY_THRESHOLD = 2048;

// Network connection object

/* Warning: It is very easy to accidentally introduce race conditions to linux_raw_tcp_conn_t.
Think carefully before changing read_internal(), perform_write(), or on_shutdown_*(). */

fd_t linux_raw_tcp_conn_t::connect_to(const char *host, int port) {
    std::string last_error_str;
    struct addrinfo *res;
    char port_str[10];
    scoped_fd_t sock;

    // Convert port to a string
    snprintf(port_str, 10, "%d", port);

    if (getaddrinfo(host, port_str, NULL, &res) != 0) {
        logERR("Failed to look up address %s:%d.\n", host, port);
        throw connect_failed_exc_t();
    }

    // Attempt to connect to each address found for the hostname
    for (struct addrinfo *curr = res; curr != NULL; curr = curr->ai_next)
    {
        sock.reset(socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol));
        if (sock.get() == INVALID_FD) {
            logERR("Failed to create a socket with error: %s\n", strerror(errno));
            continue;
        }
        if (connect(sock.get(), curr->ai_addr, curr->ai_addrlen) == 0)
            break; // Success

        logERR("Failed to make a connection with error: %s\n", strerror(errno));
        sock.reset();
    }

    if (sock.get() == INVALID_FD) {
        freeaddrinfo(res);
        throw connect_failed_exc_t();
    }

    guarantee_err(fcntl(sock.get(), F_SETFL, O_NONBLOCK) == 0, "Could not make socket non-blocking");

    freeaddrinfo(res);
    return sock.release();
}

fd_t linux_raw_tcp_conn_t::connect_to(const ip_address_t &host, int port) {
    scoped_fd_t sock(socket(AF_INET, SOCK_STREAM, 0));

    if (sock.get() == INVALID_FD) {
        logERR("Failed to create a socket with error: %s\n", strerror(errno));
        throw connect_failed_exc_t();
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = host.addr;
    bzero(addr.sin_zero, sizeof(addr.sin_zero));

    if (connect(sock.get(), reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
        logERR("Failed to make a connection with error: %s\n", strerror(errno));
        throw connect_failed_exc_t();
    }

    guarantee_err(fcntl(sock.get(), F_SETFL, O_NONBLOCK) == 0, "Could not make socket non-blocking");
    return sock.release();
}

linux_raw_tcp_conn_t::linux_raw_tcp_conn_t(const char *host, int port) :
    write_perfmon(NULL),
    sock(connect_to(host, port)),
    read_vmsplice_pipe(),
    write_vmsplice_pipe(),
    event_watcher(new linux_event_watcher_t(sock.get(), this)),
    read_in_progress(false),
    write_in_progress(false),
    timer_token(NULL),
    total_bytes_written(0),
    shutdown_coro(NULL),
    total_bytes_to_write(0)
{
    initialize_vmsplice_pipes();
}

linux_raw_tcp_conn_t::linux_raw_tcp_conn_t(const ip_address_t &host, int port) :
    write_perfmon(NULL),
    sock(connect_to(host, port)),
    read_vmsplice_pipe(),
    write_vmsplice_pipe(),
    event_watcher(new linux_event_watcher_t(sock.get(), this)),
    read_in_progress(false),
    write_in_progress(false),
    timer_token(NULL),
    total_bytes_written(0),
    shutdown_coro(NULL),
    total_bytes_to_write(0)
{
    initialize_vmsplice_pipes();
}

linux_raw_tcp_conn_t::linux_raw_tcp_conn_t(fd_t s) :
    write_perfmon(NULL),
    sock(s),
    read_vmsplice_pipe(),
    write_vmsplice_pipe(),
    event_watcher(new linux_event_watcher_t(sock.get(), this)),
    read_in_progress(false),
    write_in_progress(false),
    timer_token(NULL),
    total_bytes_written(0),
    shutdown_coro(NULL),
    total_bytes_to_write(0)
{
    rassert(sock.get() != INVALID_FD);

    // TODO: is this throwing away previous flags?
    int res = fcntl(sock.get(), F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make socket non-blocking");
    initialize_vmsplice_pipes();
}

void linux_raw_tcp_conn_t::initialize_vmsplice_pipes() {
    fd_t pipe[2];
    if (pipe2(pipe, O_NONBLOCK | O_CLOEXEC) != 0) {
        crash("Could not create pipe"); // RSI: handle the pipe creation failure gracefully
    } else {
        read_vmsplice_pipe.reset(pipe[0]);
        write_vmsplice_pipe.reset(pipe[1]);
    }
}

void linux_raw_tcp_conn_t::timer_hook(void *instance) {
    reinterpret_cast<linux_raw_tcp_conn_t *>(instance)->timer_handle_callbacks();
}

void linux_raw_tcp_conn_t::timer_handle_callbacks() {
    // Get the current unsent bytes from the tcp queue
    int send_queue_size;
    int result = ::ioctl(sock.get(), SIOCOUTQ, &send_queue_size);
    if (result != 0) {
        logERR("Failed to get SIOCOUTQ from stream: %s\n", strerror(errno));
        return;
    }

    // Call any callbacks whose buffers have been flushed
    size_t current_position = total_bytes_written - send_queue_size;
    remove_write_callbacks(current_position);
}

void linux_raw_tcp_conn_t::add_write_callback(write_callback_t *callback) {
    // Set up the timer for running write callbacks
    if (timer_token == NULL) {
        timer_token = linux_thread_pool_t::thread->timer_handler.add_timer_internal(1, &timer_hook, this, false);
        rassert(timer_token != NULL);
    }

    callback->position_in_stream = total_bytes_written;
    write_callback_list.push_back(callback);
}

void linux_raw_tcp_conn_t::remove_write_callbacks(size_t current_position) {
    while (!write_callback_list.empty()) {
        write_callback_t *callback = write_callback_list.front();
        if (current_position >= callback->position_in_stream) {
            write_callback_list.pop_front();
            callback->done();
        } else {
            break;
        }
    }

    // Check if we should remove timer from timer handler
    if (write_callback_list.empty()) {
        rassert(timer_token != NULL);
        linux_thread_pool_t::thread->timer_handler.cancel_timer(timer_token);
        timer_token = NULL;

        if (shutdown_coro != NULL) {
            shutdown_coro->notify_later();
        }
    }
}

// TODO: if this fails because the connection dies, do anything?
void linux_raw_tcp_conn_t::write(const void *buffer, size_t size, write_callback_t *callback) THROWS_ONLY(write_closed_exc_t) {
    struct iovec iov = { const_cast<void *>(buffer), size };
    write_vectored(&iov, 1, callback);
}

void linux_raw_tcp_conn_t::write_vectored(struct iovec *iov, size_t count, write_callback_t *callback) THROWS_ONLY(write_closed_exc_t) {
    assert_thread();
    rassert(!write_in_progress);

    // TODO: see if this is necessary
    if (write_closed.is_pulsed()) {
        /* The write end of the connection was closed, but there are still
        operations in the write queue; we are one of those operations. Just
        don't do anything. */
        return;
    }

    size_t bytes_to_write = 0;
    for (size_t i = 0; i < count; i++)
        bytes_to_write += iov[i].iov_len;

    total_bytes_to_write += bytes_to_write; // RSI: remove

    if (bytes_to_write >= ZEROCOPY_THRESHOLD && callback != NULL) {
        write_zerocopy(iov, count, bytes_to_write, callback);
    } else {
        write_copy(iov, count, bytes_to_write, callback);
    }

    rassert(total_bytes_written == total_bytes_to_write);
}

void linux_raw_tcp_conn_t::advance_iov(struct iovec *&iov, size_t &count, size_t bytes_written) {
    while (bytes_written > 0) {
        rassert(count > 0);
        size_t cur_len = std::min(iov->iov_len, bytes_written);
        bytes_written -= cur_len;
        iov->iov_len -= cur_len;
        iov->iov_base = reinterpret_cast<char *>(iov->iov_base) + cur_len;

        while (count > 0 && iov->iov_len == 0) {
            ++iov;
            --count;
        }
    }
}

void linux_raw_tcp_conn_t::write_zerocopy(struct iovec *iov, size_t count, size_t bytes_to_write, write_callback_t *callback) THROWS_ONLY(write_closed_exc_t) {
    rassert(callback != NULL);

    while (bytes_to_write > 0) {
        ssize_t vmsplice_res = vmsplice(write_vmsplice_pipe.get(), iov, count, SPLICE_F_NONBLOCK);
        if (vmsplice_res > 0) {
            size_t bytes_to_splice = static_cast<size_t>(vmsplice_res);
            rassert(bytes_to_splice <= bytes_to_write);
            bytes_to_write -= bytes_to_splice;
            advance_iov(iov, count, bytes_to_splice);

            while (bytes_to_splice > 0) {
                ssize_t splice_res = splice(read_vmsplice_pipe.get(), NULL, sock.get(), NULL, bytes_to_splice, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
                if (splice_res >= 0) {
                    rassert(static_cast<size_t>(splice_res) <= bytes_to_splice);
                    bytes_to_splice -= splice_res;
                    total_bytes_written += splice_res;
                    if (bytes_to_splice > 0) {
                        wait_for_epoll_out();
                    }
                } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    wait_for_epoll_out();
                } else {
                    int err = errno;
                    logERR("Could not splice to a socket: %s\n", strerror(err));
                    shutdown_write();
                    throw write_closed_exc_t(err);
                }
            }
        } else if (vmsplice_res == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
            wait_for_pipe_epoll_out();
        } else {
            int err = errno;
            logERR("Could not vmsplice to a pipe: %s\n", strerror(err));
            shutdown_write();
            throw write_closed_exc_t(err);
        }
    }

    add_write_callback(callback);
}

void linux_raw_tcp_conn_t::write_copy(struct iovec *iov, size_t count, size_t bytes_to_write, write_callback_t *callback) THROWS_ONLY(write_closed_exc_t) {
    while (bytes_to_write > 0) {
        int res = ::writev(sock.get(), iov, count);

        if (res >= 0) {
            size_t bytes_written = static_cast<size_t>(res);
            rassert(bytes_written <= bytes_to_write);
            bytes_to_write -= bytes_written;
            total_bytes_written += bytes_written;
            advance_iov(iov, count, bytes_written);
            if (write_perfmon)
                write_perfmon->record(res);
        } else if (res == -1 && (errno == EPIPE || errno == ENOTCONN || errno == EHOSTUNREACH ||
                                 errno == ENETDOWN || errno == EHOSTDOWN || errno == ECONNRESET)) {
            // These errors are expected to happen at some point in practice
            int err = errno;
            shutdown_write();
            throw write_closed_exc_t(err);
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            /* In theory this should never happen, but it probably will. So we write a log message
               and then shut down normally. */
            int err = errno;
            logERR("Could not write to a socket: %s\n", strerror(err));
            shutdown_write();
            throw write_closed_exc_t(err);
        } else {
            // THIS SPACE IS INTENTIONALLY LEFT BLANK:
            // do nothing in the case of EAGAIN/EWOULDBLOCK
        }

        if (bytes_to_write > 0) {
            wait_for_epoll_out();
            // Go around the loop and try to write again
        }
    }

    if (callback) {
        callback->done();
    }
}

void linux_raw_tcp_conn_t::wait_for_epoll_out() THROWS_ONLY(write_closed_exc_t) {
    rassert(!write_in_progress);
    write_in_progress = true;
    // Wait for a notification from the event queue
    cond_t cond;

    // Set up the cond so it gets pulsed when the socket is closed
    cond_link_t pulse_if_shut_down(&write_closed, &cond);

    // Set up the cond so it gets pulsed if an event comes
    event_watcher->watch(poll_event_out, boost::bind(&cond_t::pulse, &cond), &cond);

    // Wait for something to happen.
    cond.wait_lazily();

    write_in_progress = false;
    if (write_closed.is_pulsed()) {
        /* We were closed for whatever reason. Something else has already called
        on_shutdown_write(). In fact, we were probably signalled by on_shutdown_write(). */
        int err = errno;
        throw write_closed_exc_t(err);
    }
}

void linux_raw_tcp_conn_t::wait_for_pipe_epoll_out() THROWS_ONLY(write_closed_exc_t) {
    // RSI: implement this - need a new event watcher?
}

void linux_raw_tcp_conn_t::read_exactly(void *buffer, size_t size) THROWS_ONLY(read_closed_exc_t) {
    size_t bytes_read(0);
    while (bytes_read < size)
        bytes_read += read(reinterpret_cast<char *>(buffer) + bytes_read, size - bytes_read);
}

size_t linux_raw_tcp_conn_t::read(void *buffer, size_t size) THROWS_ONLY(read_closed_exc_t) {
    assert_thread();
    rassert(!read_closed.is_pulsed());

    while (true) {
        ssize_t res = read_non_blocking(buffer, size);

        if (res > 0) {
            // We read some data, whooo
            return res;
        } else {
            wait_for_epoll_in();
            // Go around the loop and try to read again
        }
    }
}

void linux_raw_tcp_conn_t::wait_for_epoll_in() THROWS_ONLY(read_closed_exc_t) {
    rassert(!read_in_progress);
    read_in_progress = true;

    /* There's no data available right now, so we must wait for a notification from the
    epoll queue. `cond` will be pulsed when the socket is closed or when there is data
    available. */
    cond_t cond;

    // Set up the cond so it gets pulsed when the socket is closed
    cond_link_t pulse_if_shut_down(&read_closed, &cond);

    // Set up the cond so it gets pulsed if an event comes
    event_watcher->watch(poll_event_in, boost::bind(&cond_t::pulse, &cond), &cond);

    /* Wait for something to happen.

    We must wait lazily because if we wait eagerly, the `linux_raw_tcp_conn_t` could be
    immediately destroyed as a consequence of our being notified, which could screw up
    the `linux_event_watcher_t`. See issue #322.

    At one time it was believed that `wait_eagerly()` would provide better performance,
    because `wait_lazily()` means an extra trip around the event loop when the connection
    becomes readable. On 2011-05-12, Tim benchmarked `wait_eagerly()` against
    `wait_lazily()` on electro with an all-get workload, no keys in the database, and
    default stress-client and server parameters, and found that it had no significant
    impact on performance. */
    cond.wait_lazily();

    read_in_progress = false;

    if (read_closed.is_pulsed()) {
        /* We were closed for whatever reason. Something else has already called
        on_shutdown_read(). In fact, we were probably signalled by on_shutdown_read(). */
        throw read_closed_exc_t();
    }
}

size_t linux_raw_tcp_conn_t::read_non_blocking(void *buffer, size_t size) THROWS_ONLY(read_closed_exc_t) {
    rassert(!read_in_progress);
    ssize_t res = ::read(sock.get(), buffer, size);

    if (res > 0) {
        return res;
    } else if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return 0;
    } else if (res == 0 || (res == -1 && (errno == ECONNRESET || errno == ENOTCONN))) {
        // We will end up here if the remote side has closed with a read result of 0
        /* We were closed. This is the first notification that the kernel has given us, so we
        must call on_shutdown_read(). */
        on_shutdown_read();
        throw read_closed_exc_t();

    } else {
        /* Unknown error. This is not expected, but it will probably happen sometime so we
        shouldn't crash. */
        logERR("Could not read from socket: %s\n", strerror(errno));
        on_shutdown_read();
        throw read_closed_exc_t();
    }
}

void linux_raw_tcp_conn_t::shutdown_read() {
    assert_thread();
    int res = ::shutdown(sock.get(), SHUT_RD);
    if (res != 0 && errno != ENOTCONN) {
        logERR("Could not shutdown socket for reading: %s\n", strerror(errno));
    }

    on_shutdown_read();
}

void linux_raw_tcp_conn_t::on_shutdown_read() {
    assert_thread();
    rassert(!read_closed.is_pulsed());
    read_closed.pulse();
}

bool linux_raw_tcp_conn_t::is_read_open() {
    assert_thread();
    return !read_closed.is_pulsed();
}

void linux_raw_tcp_conn_t::shutdown_write() {
    assert_thread();

    int res = ::shutdown(sock.get(), SHUT_WR);
    if (res != 0 && errno != ENOTCONN) {
        logERR("Could not shutdown socket for writing: %s\n", strerror(errno));
    }
    on_shutdown_write();
}

void linux_raw_tcp_conn_t::on_shutdown_write() {
    assert_thread();
    rassert(!write_closed.is_pulsed());
    write_closed.pulse();
}

bool linux_raw_tcp_conn_t::is_write_open() {
    assert_thread();
    return !write_closed.is_pulsed();
}

linux_raw_tcp_conn_t::~linux_raw_tcp_conn_t() {
    assert_thread();

    if (is_read_open()) shutdown_read();
    if (is_write_open()) shutdown_write();

    delete event_watcher;
    event_watcher = NULL;

    // Wait for callback queue to empty out before shutting down the socket
    guarantee(coro_t::self() != NULL);
    while (!write_callback_list.empty()) {
        guarantee(timer_token != NULL);
        shutdown_coro = coro_t::self();
        coro_t::wait();
    }

    // scoped_fd_t's destructor will take care of closing the socket.
}

void linux_buffered_tcp_conn_t::rethread(int new_thread) {
    conn.rethread(new_thread);
    real_home_thread = conn.real_home_thread;
}

void linux_raw_tcp_conn_t::rethread(int new_thread) {
    if (home_thread() == get_thread_id() && new_thread == INVALID_THREAD) {
        rassert(!read_in_progress);
        rassert(!write_in_progress);
        rassert(event_watcher);
        delete event_watcher;
        event_watcher = NULL;

    } else if (home_thread() == INVALID_THREAD && new_thread == get_thread_id()) {
        rassert(!event_watcher);
        event_watcher = new linux_event_watcher_t(sock.get(), this);

    } else {
        // TODO: find out why we can't rethread directly
        crash("linux_raw_tcp_conn_t can be rethread()ed from no thread to the current thread or "
            "from the current thread to no thread, but no other combination is legal. The "
            "current thread is %d; the old thread is %d; the new thread is %d.\n",
            get_thread_id(), home_thread(), new_thread);
    }

    real_home_thread = new_thread;

    read_closed.rethread(new_thread);
    write_closed.rethread(new_thread);
}

void linux_raw_tcp_conn_t::on_event(int events) {
    assert_thread();

    /* This is called by linux_event_watcher_t when error events occur. Ordinary
    poll_event_in/poll_event_out events are not sent through this function. */

    bool reading = event_watcher->is_watching(poll_event_in);
    bool writing = event_watcher->is_watching(poll_event_out);

    if (events & (poll_event_err | poll_event_hup)) {
        /* TODO: What's the significance of these 'if' statements? Do they actually make
        any sense? Why don't we just close both halves of the socket?  - probably to finish writing/reading what is left */

        if (writing) {
            /* We get this when the socket is closed but there is still data we are trying to send.
            For example, it can sometimes be reproduced by sending "nonsense\r\n" and then sending
            "set [key] 0 0 [length] noreply\r\n[value]\r\n" a hundred times then immediately closing
            the socket.

            I speculate that the "error" part comes from the fact that there is undelivered data
            in the socket send buffer, and the "hup" part comes from the fact that the remote end
            has hung up.

            The same can happen for reads, see next case. */

            on_shutdown_write();
        }

        if (reading) {
            // See description for write case above
            on_shutdown_read();
        }

        if (!reading && !writing) {
            /* We often get a combination of poll_event_err and poll_event_hup when a socket
            suddenly disconnects. It seems safe to assume it just indicates a hang-up. */
            if (!read_closed.is_pulsed()) shutdown_read();
            if (!write_closed.is_pulsed()) shutdown_write();
        }

    } else {
        // We don't know why we got this, so log it and then shut down the socket
        logERR("Unexpected epoll err/hup/rdhup. events=%s, reading=%s, writing=%s\n",
            format_poll_event(events).c_str(),
            reading ? "yes" : "no",
            writing ? "yes" : "no");
        if (!read_closed.is_pulsed()) shutdown_read();
        if (!write_closed.is_pulsed()) shutdown_write();
    }
}

linux_buffered_tcp_conn_t::linux_buffered_tcp_conn_t(const char *host, int port) :
    read_buffer_offset(0),
    conn(host, port),
    write_buffer(MAX_WRITE_BUFFER_SIZE)
{
    read_buffer.reserve(MIN_READ_BUFFER_SIZE);
}

linux_buffered_tcp_conn_t::linux_buffered_tcp_conn_t(const ip_address_t &host, int port) :
    read_buffer_offset(0),
    conn(host, port),
    write_buffer(MAX_WRITE_BUFFER_SIZE)
{
    read_buffer.reserve(MIN_READ_BUFFER_SIZE);
}

linux_buffered_tcp_conn_t::linux_buffered_tcp_conn_t(fd_t sock) :
    read_buffer_offset(0),
    conn(sock),
    write_buffer(MAX_WRITE_BUFFER_SIZE)
{
    read_buffer.reserve(MIN_READ_BUFFER_SIZE);
}

char * linux_buffered_tcp_conn_t::get_read_buffer_start() {
    return read_buffer.data() + read_buffer_offset;
}

char * linux_buffered_tcp_conn_t::get_read_buffer_end() {
    return read_buffer.data() + read_buffer.size();
}

void linux_buffered_tcp_conn_t::read_exactly(void *buf, size_t size) THROWS_ONLY(read_closed_exc_t) {
    size_t bytes_read = 0;
    size_t buffered_bytes = read_buffer.size() - read_buffer_offset;

    if (buffered_bytes > 0) {
        size_t bytes_to_copy = std::min(buffered_bytes, size);
        memcpy(buf, get_read_buffer_start(), bytes_to_copy);
        bytes_read += bytes_to_copy;
        pop_read_buffer(bytes_to_copy);
    }

    conn.read_exactly(reinterpret_cast<char *>(buf) + bytes_read, size - bytes_read);
}

void linux_buffered_tcp_conn_t::read_more_buffered() THROWS_ONLY(read_closed_exc_t) {
    // Make sure there is enough free room in the buffer (may need to be expanded or shifted)
    const size_t free_space = read_buffer.capacity() - read_buffer.size();
    if (free_space < MIN_BUFFERED_READ_SIZE) {
        size_t data_size = get_read_buffer_end() - get_read_buffer_start();

        // If we can get the needed size by shifting the current buffer, do so, otherwise reallocate
        if (read_buffer_offset + free_space > MIN_BUFFERED_READ_SIZE) {
            memmove(read_buffer.data(), get_read_buffer_start(), data_size);
            read_buffer.resize(read_buffer.size() - read_buffer_offset);
        } else { // Not enough space, expand buffer
            std::vector<char> temp_buffer;
            size_t new_size = read_buffer.capacity() * READ_BUFFER_RESIZE_FACTOR;
            temp_buffer.reserve(new_size);
            temp_buffer.resize(data_size); // RSI: this is inefficient
            memcpy(temp_buffer.data(), get_read_buffer_start(), data_size);

            read_buffer.swap(temp_buffer);
        }
        read_buffer_offset = 0;
    }

    // Read available data into read buffer
    size_t old_size = read_buffer.size();
    read_buffer.resize(read_buffer.capacity());
    size_t read_bytes = conn.read(read_buffer.data() + old_size, read_buffer.size() - old_size);
    read_buffer.resize(old_size + read_bytes);
}

const_charslice linux_buffered_tcp_conn_t::peek_read_buffer() {
    conn.assert_thread();
    return const_charslice(get_read_buffer_start(), get_read_buffer_end());
}

void linux_buffered_tcp_conn_t::pop_read_buffer(size_t len) {
    conn.assert_thread();

    size_t occupied_bytes = read_buffer.size() - read_buffer_offset;
    rassert(len <= occupied_bytes);
    read_buffer_offset += len;

    // Check if buffer utilization is less than threshold and reduce size if it is
    if (READ_BUFFER_MIN_UTILIZATION_FACTOR * occupied_bytes < read_buffer.capacity()) {
        // We're only using a fraction of the buffer, shrink it
        size_t new_size = std::max(read_buffer.capacity() / READ_BUFFER_RESIZE_FACTOR, MIN_READ_BUFFER_SIZE);
        if (new_size < read_buffer.capacity()) {
            std::vector<char> temp_buffer;
            temp_buffer.reserve(new_size);
            size_t data_size = read_buffer.size() - read_buffer_offset;
            temp_buffer.resize(data_size); // RSI: this is inefficient
            memcpy(temp_buffer.data(), get_read_buffer_start(), data_size);

            read_buffer.swap(temp_buffer);
            read_buffer_offset = 0;
        }
    }
}

void linux_buffered_tcp_conn_t::write(const void *buf, size_t size) THROWS_ONLY(write_closed_exc_t) {
    conn.assert_thread();

    // Copy what will fit of buf into write buffer
    size_t total_write_size = write_buffer.size + size;
    if (total_write_size <= MAX_WRITE_BUFFER_SIZE) {
        write_buffer.put_back(buf, size);
    } else {
        struct iovec iov[2] = { { write_buffer.data, write_buffer.size },
                                { const_cast<void *>(buf), size } };
        conn.write_vectored(iov, 2, NULL);
        write_buffer.clear();
    }
}

void linux_buffered_tcp_conn_t::write_vectored(struct iovec *iov, size_t count, write_callback_t *callback) THROWS_ONLY(write_closed_exc_t) {
    // If we don't have a pending write buffer, we can pass the write directly down
    if (write_buffer.size == 0) {
        conn.write_vectored(iov, count, callback);
    } else {
        // We have a pending write buffer, either batch it into the iovectors or flush our buffer first
        if (callback == NULL) {
            struct iovec buffered_iov[count + 1];
            memcpy(buffered_iov + 1, iov, sizeof(*iov) * count);

            buffered_iov[0].iov_base = write_buffer.data;
            buffered_iov[0].iov_len = write_buffer.size;

            conn.write_vectored(buffered_iov, count + 1, NULL);
        } else {
            // Since there is a callback, we can't allow our write_buffer to be zerocopied
            //   which would complicate the deallocation or reuse of the write_buffer
            flush_write_buffer();
            conn.write_vectored(iov, count, callback);
        }
        write_buffer.clear();
    }
}

void linux_buffered_tcp_conn_t::flush_write_buffer() THROWS_ONLY(write_closed_exc_t) {
    if (write_buffer.size > 0) {
        conn.write(write_buffer.data, write_buffer.size, NULL);
        write_buffer.clear();
    }
}

// TODO: implement passthrough for various raw connection functions (read_closed, etc), do a diff to see all fns
linux_raw_tcp_conn_t& linux_buffered_tcp_conn_t::get_raw_connection() THROWS_ONLY(write_closed_exc_t) {
    guarantee(read_buffer.size() - read_buffer_offset == 0);
    flush_write_buffer();
    return conn;
}

void linux_buffered_tcp_conn_t::shutdown_write() {
    conn.shutdown_write();
}

bool linux_buffered_tcp_conn_t::is_write_open() {
    return conn.is_write_open();
}

void linux_buffered_tcp_conn_t::shutdown_read() {
    conn.shutdown_read();
}

bool linux_buffered_tcp_conn_t::is_read_open() {
    return conn.is_read_open();
}

// Network listener object

linux_tcp_listener_t::linux_tcp_listener_t(int port,
        boost::function<void(boost::scoped_ptr<linux_buffered_tcp_conn_t>&)> cb) :
    sock(socket(AF_INET, SOCK_STREAM, 0)),
    event_watcher(sock.get(), this),
    callback(cb),
    log_next_error(true)
{
    int res;

    guarantee_err(sock.get() != INVALID_FD, "Couldn't create socket");

    int sockoptval = 1;
    res = setsockopt(sock.get(), SOL_SOCKET, SO_REUSEADDR, &sockoptval, sizeof(sockoptval));
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
    res = setsockopt(sock.get(), IPPROTO_TCP, TCP_NODELAY, &sockoptval, sizeof(sockoptval));
    guarantee_err(res != -1, "Could not set TCP_NODELAY option");

    // Bind the socket
    sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    res = bind(sock.get(), (sockaddr*)&serv_addr, sizeof(serv_addr));
    if (res != 0) {
        if (errno == EADDRINUSE) {
            throw address_in_use_exc_t();
        } else {
            crash("Could not bind socket: %s\n", strerror(errno));
        }
    }

    // Start listening to connections
    res = listen(sock.get(), 5);
    guarantee_err(res == 0, "Couldn't listen to the socket");

    res = fcntl(sock.get(), F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make socket non-blocking");

    // Start the accept loop
    accept_loop_handler.reset(new side_coro_handler_t(
        boost::bind(&linux_tcp_listener_t::accept_loop, this, _1)
        ));
}

void linux_tcp_listener_t::accept_loop(signal_t *shutdown_signal) {
    static const int initial_backoff_delay_ms = 10;   // Milliseconds
    static const int max_backoff_delay_ms = 160;
    int backoff_delay_ms = initial_backoff_delay_ms;

    while (!shutdown_signal->is_pulsed()) {
        fd_t new_sock = accept(sock.get(), NULL, NULL);

        if (new_sock != INVALID_FD) {
            coro_t::spawn_now(boost::bind(&linux_tcp_listener_t::handle, this, new_sock));

            /* If we backed off before, un-backoff now that the problem seems to be
            resolved. */
            if (backoff_delay_ms > initial_backoff_delay_ms) backoff_delay_ms /= 2;

            /* Assume that if there was a problem before, it's gone now because accept()
            is working. */
            log_next_error = true;

        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* Wait for a notification from the event loop, or for a command to shut down,
            before continuing */
            cond_t c;
            cond_link_t interrupt_wait_on_shutdown(shutdown_signal, &c);
            event_watcher.watch(poll_event_in, boost::bind(&cond_t::pulse, &c), &c);
            c.wait();

        } else if (errno == EINTR) {
            /* Harmless error; just try again. */

        } else {
            // Unexpected error. Log it unless it's a repeat error.
            if (log_next_error) {
                logERR("accept() failed: %s.\n",
                    strerror(errno));
                log_next_error = false;
            }

            /* Delay before retrying. We use pulse_after_time() instead of nap() so that we will
            be interrupted immediately if something wants to shut us down. */
            cond_t c;
            cond_link_t interrupt_wait_on_shutdown(shutdown_signal, &c);
            call_with_delay(backoff_delay_ms, boost::bind(&cond_t::pulse, &c), &c);
            c.wait();

            // Exponentially increase backoff time
            if (backoff_delay_ms < max_backoff_delay_ms) backoff_delay_ms *= 2;
        }
    }
}

void linux_tcp_listener_t::handle(fd_t socket) {
    boost::scoped_ptr<linux_buffered_tcp_conn_t> conn(new linux_buffered_tcp_conn_t(socket));
    callback(conn);
}

linux_tcp_listener_t::~linux_tcp_listener_t() {
    // Interrupt the accept loop
    accept_loop_handler.reset();

    int res;

    res = shutdown(sock.get(), SHUT_RDWR);
    guarantee_err(res == 0, "Could not shutdown main socket");

    // scoped_fd_t destructor will close() the socket
}

void linux_tcp_listener_t::on_event(int events) {
    /* This is only called in cases of error; normal input events are recieved
    via event_listener.watch(). */

    if (log_next_error) {
        logERR("poll()/epoll() sent linux_tcp_listener_t errors: %d.\n", events);
        log_next_error = false;
    }
}

linux_buffered_tcp_conn_t::write_buffer_t::write_buffer_t(size_t size_) : data(static_cast<char*>(malloc_aligned(size_, getpagesize()))), size(0), capacity(size_) { }

linux_buffered_tcp_conn_t::write_buffer_t::~write_buffer_t() {
    free(data);
}

void linux_buffered_tcp_conn_t::write_buffer_t::clear() {
    size = 0;
}

void linux_buffered_tcp_conn_t::write_buffer_t::put_back(const void * buf, size_t sz) {
    rassert(size + sz <= capacity);
    memcpy(data + size, buf, sz);
    size += sz;
}

