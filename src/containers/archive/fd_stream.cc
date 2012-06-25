#include "containers/archive/fd_stream.hpp"

#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "arch/fd_send_recv.hpp"
#include "arch/runtime/event_queue.hpp" // format_poll_event
#include "concurrency/wait_any.hpp"     // wait_any_t
#include "errors.hpp"
#include "logger.hpp"  // logERR

// -------------------- blocking_fd_watcher_t --------------------
blocking_fd_watcher_t::blocking_fd_watcher_t()
    : read_open_(true), write_open_(true) {}

bool blocking_fd_watcher_t::is_read_open() { return read_open_; }
bool blocking_fd_watcher_t::is_write_open() { return write_open_; }

void blocking_fd_watcher_t::on_shutdown_read() {
    guarantee(read_open_);
    read_open_ = false;
}

void blocking_fd_watcher_t::on_shutdown_write() {
    guarantee(write_open_);
    write_open_ = false;
}

bool blocking_fd_watcher_t::wait_for_read() {
    guarantee(read_open_); 
    return true;
}

bool blocking_fd_watcher_t::wait_for_write() {
    guarantee(write_open_); 
    return true;
}

void blocking_fd_watcher_t::init_callback(UNUSED linux_event_callback_t *cb) {}


// -------------------- linux_event_fd_watcher_t --------------------
linux_event_fd_watcher_t::linux_event_fd_watcher_t(fd_t fd)
    : io_in_progress_(false),
      event_callback_(NULL),
      event_watcher_(new linux_event_watcher_t(fd, this))
{
    guarantee_err(0 == fcntl(fd, F_SETFL, O_NONBLOCK),
                  "Could not make fd non-blocking.");
}

void linux_event_fd_watcher_t::init_callback(linux_event_callback_t *cb) {
    guarantee(!event_callback_);
    guarantee(cb);
    event_callback_ = cb;
}

void linux_event_fd_watcher_t::on_event(int events) {
    assert_thread();
    guarantee(event_callback_);
    event_callback_->on_event(events);
}

bool linux_event_fd_watcher_t::is_read_open() { return !read_closed_.is_pulsed(); }
bool linux_event_fd_watcher_t::is_write_open() { return !write_closed_.is_pulsed(); }

void linux_event_fd_watcher_t::on_shutdown_read() {
    assert_thread();
    rassert(is_read_open());
    read_closed_.pulse();
}

void linux_event_fd_watcher_t::on_shutdown_write() {
    assert_thread();
    rassert(is_write_open());
    write_closed_.pulse();
}

bool linux_event_fd_watcher_t::wait_for_read() {
    // Wait for a notification from the event queue, or an order to shut down
    assert_thread();
    guarantee(!io_in_progress_);
    io_in_progress_ = true;

    linux_event_watcher_t::watch_t watch(event_watcher_.get(), poll_event_in);
    wait_any_t waiter(&watch, &read_closed_);
    waiter.wait_lazily_unordered();

    guarantee(io_in_progress_);
    io_in_progress_ = false;

    return is_read_open();
}

bool linux_event_fd_watcher_t::wait_for_write() {
    // Wait for a notification from the event queue, or an order to shut down
    assert_thread();
    guarantee(!io_in_progress_);
    io_in_progress_ = true;

    linux_event_watcher_t::watch_t watch(event_watcher_.get(), poll_event_out);
    wait_any_t waiter(&watch, &write_closed_);
    waiter.wait_lazily_unordered();

    guarantee(io_in_progress_);
    io_in_progress_ = false;

    return is_write_open();
}

linux_event_fd_watcher_t::~linux_event_fd_watcher_t() {
    assert_thread();

    rassert(!is_read_open());
    rassert(!is_write_open());
}


// -------------------- fd_stream_t --------------------
fd_stream_t::fd_stream_t(int fd, fd_watcher_t *watcher)
    : fd_(fd),
      fd_watcher_(watcher ? watcher : new linux_event_fd_watcher_t(fd))
{
    fd_watcher_->init_callback(this);
}

fd_stream_t::~fd_stream_t() {
    assert_thread();

    // See comment in fd_stream.hpp.
    rassert(!fd_watcher_->is_read_open());
    rassert(!fd_watcher_->is_write_open());
}

int64_t fd_stream_t::read(void *buf, int64_t size) {
    assert_thread();
    guarantee(size > 0);

    if (!fd_watcher_->is_read_open())
        return -1;

    char *bufp = static_cast<char *>(buf);
    for (;;) {
        rassert(fd_watcher_->is_read_open());

        ssize_t res = ::read(fd_.get(), bufp, size);

        if (res > 0) {
            // We read some data.
            guarantee(res <= size);
            return res;

        } else if (res == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Wait until we can read, or we shut down.
            if (!fd_watcher_->wait_for_read())
                return -1;          // we shut down.

            // Go around the loop to try to read again

        } else {
            // EOF or error
            if (res == -1)
                on_read_error(errno);
            else
                guarantee(res == 0); // sanity
            shutdown_read();
            return -1;
        }
    }
}

int64_t fd_stream_t::write(const void *buf, int64_t size) {
    assert_thread();
    guarantee(size > 0);

    if (!fd_watcher_->is_write_open())
        return -1;

    int64_t orig_size = size;
    const char *bufp = static_cast<const char *>(buf);
    while (size > 0) {
        rassert(fd_watcher_->is_write_open());

        ssize_t res = ::write(fd_.get(), bufp, size);

        if (res > 0) {
            // We wrote something!
            guarantee(res <= size);
            bufp += res;
            size -= res;
            // Go around the loop; keep writing until done.

        } else if (res == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Wait until we can write, or we shut down.
            if (!fd_watcher_->wait_for_write())
                return -1;      // we shut down.
            // Go around the loop and write some more.

        } else {
            if (res == -1)
                on_write_error(errno);
            else
                logERR("Didn't expect write() to return %ld.", res);
            shutdown_write();
            return -1;
        }
    }

    return orig_size;
}

void fd_stream_t::shutdown_read() {
    assert_thread();
    do_shutdown_read();
    if (fd_watcher_->is_read_open())
        fd_watcher_->on_shutdown_read();
}

void fd_stream_t::shutdown_write() {
    assert_thread();
    do_shutdown_write();
    if (fd_watcher_->is_write_open())
        fd_watcher_->on_shutdown_write();
}

void fd_stream_t::on_event(int events) {
    assert_thread();

    /* This is called by linux_event_watcher_t when error events occur. Ordinary
    poll_event_in/poll_event_out events are not sent through this function. */

    if (!(events & (poll_event_err | poll_event_hup))) {
        // It wasn't an error or hangup, so why did we fail? Log it.
        std::string s(format_poll_event(events));
        logERR("Unexpected epoll err/hup/rdhup. events=%s", s.c_str());
    }

    // Shut down.
    if (fd_watcher_->is_write_open()) shutdown_write();
    if (fd_watcher_->is_read_open()) shutdown_read();
}


// -------------------- socket_stream_t --------------------
socket_stream_t::socket_stream_t(int fd, fd_watcher_t *watcher)
    : fd_stream_t(fd, watcher) {}

socket_stream_t::~socket_stream_t() {
    if (fd_watcher_->is_read_open()) shutdown_read();
    if (fd_watcher_->is_write_open()) shutdown_write();
}

void socket_stream_t::on_read_error(int errsv) {
    if (errsv != EPIPE && errsv != ECONNRESET && errsv != ENOTCONN) {
        // Unexpected error (not just "we closed").
        logERR("Could not read from socket: %s", strerror(errsv));
    }
}

void socket_stream_t::on_write_error(int errsv) {
    if (errsv != EPIPE && errsv != ENOTCONN && errsv != ECONNRESET) {
        // Unexpected error (not just "we closed")
        logERR("Could not read from socket: %s", strerror(errsv));
    }
}

void socket_stream_t::do_shutdown_read() {
    assert_thread();
    int res = ::shutdown(fd_.get(), SHUT_RD);
    if (res != 0 && errno != ENOTCONN)
        logERR("Could not shutdown socket for reading: %s", strerror(errno));
}

void socket_stream_t::do_shutdown_write() {
    assert_thread();
    int res = ::shutdown(fd_.get(), SHUT_WR);
    if (res != 0 && errno != ENOTCONN)
        logERR("Could not shutdown socket for writing: %s", strerror(errno));
}


// -------------------- unix_socket_stream_t --------------------
unix_socket_stream_t::unix_socket_stream_t(int fd, fd_watcher_t *watcher)
    : socket_stream_t(fd, watcher) {}

int unix_socket_stream_t::send_fd(int fd) {
    return send_fds(1, &fd);
}

archive_result_t unix_socket_stream_t::recv_fd(int *fd) {
    return recv_fds(1, fd);
}

int unix_socket_stream_t::send_fds(size_t num_fds, int *fds) {
    while (-1 == ::send_fds(fd_.get(), num_fds, fds)) {
        if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
            on_write_error(errno);
            return -1;
        }

        // Wait until we can write, or we shut down.
        if (!fd_watcher_->wait_for_write())
            return -1;          // we shut down
    }

    return 0;
}

archive_result_t unix_socket_stream_t::recv_fds(size_t num_fds, int *fds) {
    for (;;)
        switch (::recv_fds(fd_.get(), num_fds, fds)) {
          case FD_RECV_OK: return ARCHIVE_SUCCESS;

          case FD_RECV_EOF: return ARCHIVE_SOCK_EOF;

          case FD_RECV_INVALID: return ARCHIVE_RANGE_ERROR;

          case FD_RECV_ERROR:
            if ((errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                // Wait until we can read, or we shut down.
                && fd_watcher_->wait_for_read())
            {
                // Go around the loop and try again.
                continue;
            }

            // Socket error, or we shut down.
            return ARCHIVE_SOCK_ERROR;

          default: unreachable();
        }
}
