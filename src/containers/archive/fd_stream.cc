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
      io_in_progress_(false)
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
    guarantee(!io_in_progress_);

    if (!is_read_open())
        return -1;

    char *bufp = static_cast<char *>(buf);
    rassert(is_read_open());
    guarantee(!io_in_progress_);

    ssize_t res = ::read(fd_.get(), bufp, size);

    if (res == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
        io_in_progress_ = true;

        linux_event_watcher_t::watch_t watch(event_watcher_.get(), poll_event_in);
        wait_any_t waiter(&watch, &read_closed_);
        waiter.wait_lazily_unordered();

        io_in_progress_ = false;

        if (read_closed_.is_pulsed()) {
            // We were closed. Something else has already called
            // on_shutdown_read(), which is probably what pulsed us.
            return -1;
        }

    } else if (res == 0 || res == -1) {
        if (res == -1)
            on_read_error(errno);
        on_shutdown_read();
    }

    return res;
}

int64_t fd_stream_t::write(const void *buf, int64_t size) {
    assert_thread();
    guarantee(!io_in_progress_);
    int64_t orig_size = size;

    if (!is_write_open())
        return -1;

    const char *bufp = static_cast<const char *>(buf);
    while (size > 0) {
        rassert(is_write_open());
        guarantee(!io_in_progress_);

        ssize_t res = ::write(fd_.get(), bufp, size);

        if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Wait for a notification from the event queue, or for an order to
            // shut down
            io_in_progress_ = true;

            linux_event_watcher_t::watch_t watch(event_watcher_.get(), poll_event_out);
            wait_any_t waiter(&watch, &write_closed_);
            waiter.wait_lazily_unordered();

            io_in_progress_ = false;

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

        guarantee(res > 0 && res <= size);
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

    if (!(events & (poll_event_err | poll_event_hup))) {
        // It wasn't an error or hangup, so why did we fail? Log it.
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
