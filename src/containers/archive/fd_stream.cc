#include "containers/archive/fd_stream.hpp"

#include "arch/runtime/event_queue.hpp" // format_poll_event
#include "concurrency/wait_any.hpp"     // wait_any_t
#include "errors.hpp"
#include "logger.hpp"  // logERR

#include <cstring>
#include <fcntl.h>
#include <sys/socket.h> // shutdown
#include <unistd.h>

// -------------------- fd_stream_t --------------------
fd_stream_t::fd_stream_t(int fd)
    : fd_(fd),
      event_watcher_(new linux_event_watcher_t(fd, this)),
      DEBUG_ONLY(io_in_progress_(false))
{
    guarantee_err(0 == fcntl(fd, F_SETFL, O_NONBLOCK),
                  "Could not make fd non-blocking.");
}

fd_stream_t::~fd_stream_t() {
    assert_thread();

    if (is_read_open()) shutdown_read();
    if (is_write_open()) shutdown_write();

    event_watcher_.reset();
}

int64_t fd_stream_t::read(void *buf, int64_t size) {
    assert_thread();
    rassert(!io_in_progress_);
    int64_t orig_size = size;

    if (!is_read_open())
        return -1;

    char *bufp = reinterpret_cast<char*>(buf);
    while (size > 0) {
        rassert(is_read_open());
        rassert(!io_in_progress_);

        ssize_t res = ::read(fd_.get(), reinterpret_cast<void*>(bufp), size);

        if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            DEBUG_ONLY_CODE(io_in_progress_ = true);

            linux_event_watcher_t::watch_t watch(event_watcher_.get(), poll_event_in);
            wait_any_t waiter(&watch, &read_closed_);
            waiter.wait_lazily_unordered();

            DEBUG_ONLY_CODE(io_in_progress_ = false);

            if (!is_read_open()) {
                // We were closed. Something else has already called
                // on_shutdown_read(), which is probably what pulsed us.
                return -1;
            }

            // Go around the loop and try to read again.

        } else if (res == 0 || res == -1) {
            if (res == -1)
                on_read_error(errno);
            on_shutdown_read();
            return -1;
        }

        rassert (res > 0 && res <= size);
        size -= res;
        bufp += res;
    }

    return orig_size;
}

int64_t fd_stream_t::write(const void *buf, int64_t size) {
    assert_thread();
    rassert(!io_in_progress_);
    int64_t orig_size = size;

    if (!is_write_open())
        return -1;

    const char *bufp = reinterpret_cast<const char*>(buf);
    while (size > 0) {
        rassert(is_write_open());
        rassert(!io_in_progress_);

        ssize_t res = ::write(fd_.get(), reinterpret_cast<const void*>(bufp), size);

        if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Wait for a notification from the event queue, or for an order to
            // shut down
            DEBUG_ONLY_CODE(io_in_progress_ = true);

            linux_event_watcher_t::watch_t watch(event_watcher_.get(), poll_event_out);
            wait_any_t waiter(&watch, &write_closed_);
            waiter.wait_lazily_unordered();

            DEBUG_ONLY_CODE(io_in_progress_ = false);

            if (!is_write_open()) {
                // We were closed. Something else has already called
                // on_shutdown_write(), which is probably what pulsed us.
                return -1;
            }

            // Go around the loop and write some more.

        } else if (res == -1 || res == 0) {
            if (res == -1)
                on_write_error(errno);
            else
                logERR("Didn't expect write() to return 0.");
            on_shutdown_write();
            return -1;
        }

        rassert (res > 0 && res <= size);
        bufp += res;
        size -= res;
    }

    return orig_size;
}

bool fd_stream_t::is_read_open() { return !read_closed_.is_pulsed(); }
bool fd_stream_t::is_write_open() { return !write_closed_.is_pulsed(); }

void fd_stream_t::shutdown_read() {
    assert_thread();
    do_shutdown_read();
    on_shutdown_read();
}

void fd_stream_t::shutdown_write() {
    assert_thread();
    do_shutdown_write();
    on_shutdown_write();
}

void fd_stream_t::on_read_error(int errno_) {
    // Default handler logs all errors.
    rassert (errno_ != EAGAIN && errno_ != EWOULDBLOCK);
    logERR("Could not read from fd: %s", strerror(errno_));
}

void fd_stream_t::on_write_error(int errno_) {
    // Default handler logs all errors.
    rassert (errno_ != EAGAIN && errno_ != EWOULDBLOCK);
    logERR("Could not write to fd: %s", strerror(errno_));
}

// NOPs by default
void fd_stream_t::do_shutdown_read() {}
void fd_stream_t::do_shutdown_write() {}

void fd_stream_t::on_shutdown_read() {
    assert_thread();
    rassert(is_read_open());
    read_closed_.pulse();
}

void fd_stream_t::on_shutdown_write() {
    assert_thread();
    rassert(is_write_open());
    write_closed_.pulse();
}

void fd_stream_t::on_event(int events) {
    assert_thread();

    /* This is called by linux_event_watcher_t when error events occur. Ordinary
    poll_event_in/poll_event_out events are not sent through this function. */

    // If it was an error or hangup (other end closed?), silently die.
    if (!(events & poll_event_err || events & poll_event_hup)) {
        // Don't know why we got this. Log it.
        std::string s(format_poll_event(events));
        logERR("Unexpected epoll err/hup/rdhup. events=%s", s.c_str());
    }

    // Shut down.
    if (is_write_open()) shutdown_write();
    if (is_read_open()) shutdown_read();
}


// -------------------- socket_stream_t --------------------
socket_stream_t::socket_stream_t(int fd)
    : fd_stream_t(fd) {}

void socket_stream_t::on_read_error(int errno_) {
    if (errno_ != ECONNRESET && errno_ != ENOTCONN) {
        // Unexpected error (not just "we closed").
        logERR("Could not read from socket: %s", strerror(errno_));
    }
}

void socket_stream_t::on_write_error(int errno_) {
    /* These errors are expected to happen at some point in practice */
    if (errno_ != EPIPE && errno_ != ENOTCONN && errno_ != EHOSTUNREACH &&
        errno_ != ENETDOWN && errno_ != EHOSTDOWN && errno_ != ECONNRESET)
    {
        // Unexpected error
        logERR("Could not read from socket: %s", strerror(errno_));
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
