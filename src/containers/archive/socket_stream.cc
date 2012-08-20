#include "containers/archive/socket_stream.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

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

bool blocking_fd_watcher_t::wait_for_read(signal_t *interruptor) {
    guarantee(read_open_);
    // blocking IO and interruption don't go together nicely
    guaranteef(!interruptor, "blocking_fd_watcher_t doesn't support interruption");
    return true;
}

bool blocking_fd_watcher_t::wait_for_write(signal_t *interruptor) {
    guarantee(write_open_);
    // blocking IO and interruption don't go together nicely
    guaranteef(!interruptor, "blocking_fd_watcher_t doesn't support interruption");
    return true;
}

void blocking_fd_watcher_t::init_callback(UNUSED linux_event_callback_t *cb) {}


// -------------------- linux_event_fd_watcher_t --------------------
linux_event_fd_watcher_t::linux_event_fd_watcher_t(fd_t fd)
    : io_in_progress_(false),
      event_callback_(NULL),
      event_watcher_(fd, this)
{
    guaranteef_err(0 == fcntl(fd, F_SETFL, O_NONBLOCK),
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

bool linux_event_fd_watcher_t::wait_for_read(signal_t *interruptor) {
    // Wait for a notification from the event queue, or an order to shut down
    assert_thread();
    guarantee(!io_in_progress_);
    io_in_progress_ = true;

    linux_event_watcher_t::watch_t watch(&event_watcher_, poll_event_in);
    wait_any_t waiter(&watch, &read_closed_);
    if (interruptor) waiter.add(interruptor);
    waiter.wait_lazily_unordered();

    guarantee(io_in_progress_);
    io_in_progress_ = false;

    if (interruptor && interruptor->is_pulsed())
        throw interrupted_exc_t();

    return is_read_open();
}

bool linux_event_fd_watcher_t::wait_for_write(signal_t *interruptor) {
    // Wait for a notification from the event queue, or an order to shut down
    assert_thread();
    guarantee(!io_in_progress_);
    io_in_progress_ = true;

    linux_event_watcher_t::watch_t watch(&event_watcher_, poll_event_out);
    wait_any_t waiter(&watch, &write_closed_);
    if (interruptor) waiter.add(interruptor);
    waiter.wait_lazily_unordered();

    guarantee(io_in_progress_);
    io_in_progress_ = false;

    if (interruptor && interruptor->is_pulsed())
        throw interrupted_exc_t();

    return is_write_open();
}

linux_event_fd_watcher_t::~linux_event_fd_watcher_t() {
    assert_thread();

    rassert(!is_read_open());
    rassert(!is_write_open());
}


// -------------------- socket_stream_t --------------------
socket_stream_t::socket_stream_t(scoped_fd_t *fd, fd_watcher_t *watcher)
    : fd_(fd->release()),
      fd_watcher_(watcher ? watcher : new linux_event_fd_watcher_t(fd_.get()))
{
    guarantee(fd_.get() != INVALID_FD);
    fd_watcher_->init_callback(this);
}

socket_stream_t::~socket_stream_t() {
    assert_thread();

    if (is_read_open()) shutdown_read();
    if (is_write_open()) shutdown_write();
}

int64_t socket_stream_t::read_interruptible(void *buf, int64_t size, signal_t *interruptor) {
    assert_thread();
    guarantee(size > 0);

    if (!check_can_read(interruptor))
        return -1;

    char *bufp = static_cast<char *>(buf);
    for (;;) {
        rassert(is_read_open());

        ssize_t res = ::read(fd_.get(), bufp, size);

        if (res > 0) {
            // We read some data.
            guarantee(res <= size);
            return res;

        } else if (res == -1 && errno == EINTR) {
            // Go around loop & retry system call.

        } else if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Wait until we can read, or we shut down, or we're interrupted.
            if (!wait_for_read(interruptor))
                return -1;      // we shut down.
            // Go around the loop to try to read again

        } else {
            // EOF or error
            if (res == -1) {
                if (errno != EPIPE && errno != ECONNRESET && errno != ENOTCONN) {
                    // Unexpected error (not just "we closed").
                    logERR("Could not read from socket: %s", strerror(errno));
                }
            } else {
                guarantee(res == 0); // sanity
            }

            shutdown_read();
            return res;
        }
    }
}

int64_t socket_stream_t::write_interruptible(const void *buf, int64_t size, signal_t *interruptor) {
    assert_thread();
    guarantee(size > 0);

    if (!check_can_write(interruptor))
        return -1;

    int64_t orig_size = size;
    const char *bufp = static_cast<const char *>(buf);
    while (size > 0) {
        rassert(is_write_open());

        ssize_t res = ::write(fd_.get(), bufp, size);

        if (res > 0) {
            // We wrote something!
            guarantee(res <= size);
            bufp += res;
            size -= res;
            // Go around the loop; keep writing until done.

        } else if (res == -1 && errno == EINTR) {
            // Go around the loop & retry system call

        } else if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Wait until we can write, or we shut down, or we're interrupted.
            if (!wait_for_write(interruptor))
                return -1;      // we shut down.
            // Go around the loop and write some more.

        } else {
            if (res == -1) {
                if (errno != EPIPE && errno != ENOTCONN && errno != ECONNRESET) {
                    // Unexpected error (not just "we closed")
                    logERR("Could not write to socket: %s", strerror(errno));
                }
            } else {
                logERR("Didn't expect write() to return %ld.", res);
            }

            shutdown_write();
            return -1;
        }
    }

    return orig_size;
}

void socket_stream_t::shutdown_read() {
    assert_thread();

    int res = ::shutdown(fd_.get(), SHUT_RD);
    if (res != 0 && errno != ENOTCONN) {
        logERR("Could not shutdown socket for reading: %s", strerror(errno));
    }

    if (fd_watcher_->is_read_open())
        fd_watcher_->on_shutdown_read();
}

void socket_stream_t::shutdown_write() {
    assert_thread();

    int res = ::shutdown(fd_.get(), SHUT_WR);
    if (res != 0 && errno != ENOTCONN) {
        logERR("Could not shutdown socket for writing: %s", strerror(errno));
    }

    if (fd_watcher_->is_write_open())
        fd_watcher_->on_shutdown_write();
}

bool socket_stream_t::wait_for_read(signal_t *interruptor) {
    try {
        return fd_watcher_->wait_for_read(interruptor);
    } catch (interrupted_exc_t) {
        shutdown_read();
        throw;
    }
}

bool socket_stream_t::wait_for_write(signal_t *interruptor) {
    try {
        return fd_watcher_->wait_for_write(interruptor);
    } catch (interrupted_exc_t) {
        shutdown_write();
        throw;
    }
}

bool socket_stream_t::check_can_read(signal_t *interruptor) {
    if (!is_read_open()) return false;
    if (interruptor && interruptor->is_pulsed()) throw interrupted_exc_t();
    return true;
}

bool socket_stream_t::check_can_write(signal_t *interruptor) {
    if (!is_write_open()) return false;
    if (interruptor && interruptor->is_pulsed()) throw interrupted_exc_t();
    return true;
}

void socket_stream_t::on_event(int events) {
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


// -------------------- unix_socket_stream_t --------------------
unix_socket_stream_t::unix_socket_stream_t(scoped_fd_t *fd, fd_watcher_t *watcher)
    : socket_stream_t(fd, watcher) {}

int unix_socket_stream_t::send_fd(int fd, signal_t *interruptor) {
    return send_fds(1, &fd, interruptor);
}

archive_result_t unix_socket_stream_t::recv_fd(int *fd, signal_t *interruptor) {
    return recv_fds(1, fd, interruptor);
}

int unix_socket_stream_t::send_fds(size_t num_fds, int *fds, signal_t *interruptor) {
    if (!check_can_write(interruptor))
        return -1;

    while (-1 == ::send_fds(fd_.get(), num_fds, fds)) {
        // Retry on interruption
        if (errno == EINTR) continue;

        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            if (errno != EPIPE && errno != ENOTCONN && errno != ECONNRESET) {
                // Unexpected error (not just "we closed")
                logERR("Could not send fds on socket: %s", strerror(errno));
            }
            shutdown_write();
            return -1;
        }

        // Wait until we can write, or we shut down, or we're interrupted.
        if (!wait_for_write(interruptor))
            return -1;          // we shut down
    }

    return 0;
}

archive_result_t unix_socket_stream_t::recv_fds(size_t num_fds, int *fds, signal_t *interruptor) {
    if (!check_can_read(interruptor))
        return ARCHIVE_SOCK_ERROR;

    for (;;) {
        switch (::recv_fds(fd_.get(), num_fds, fds)) {
          case FD_RECV_OK: return ARCHIVE_SUCCESS;

          case FD_RECV_EOF: return ARCHIVE_SOCK_EOF;

          case FD_RECV_ERROR:
            if (errno == EINTR // retry if interrupted
                || ((errno == EAGAIN || errno == EWOULDBLOCK)
                    // Wait until we can read, or we shut down.
                    && wait_for_read(interruptor)))
            {
                // Go around the loop and try again.
                continue;
            }

            // Socket error, or we shut down.
            return ARCHIVE_SOCK_ERROR;

          default: unreachable();
        }
    }
}
