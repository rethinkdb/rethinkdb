// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifdef _WIN32

#include "containers/archive/socket_stream.hpp"
#include "errors.hpp"
#include "logger.hpp"
#include "concurrency/wait_any.hpp"
#include "concurrency/interruptor.hpp"
#include "utils.hpp"

int64_t socket_stream_t::read(void *buf, int64_t count) {
    DWORD ret;
    DWORD error;
    if (event_watcher == nullptr) {
        BOOL res = ReadFile(fd, buf, count, &ret, nullptr);
        error = GetLastError();
    } else {
        overlapped_operation_t op(event_watcher);
        BOOL res = ReadFile(fd, buf, count, nullptr, &op.overlapped);
        error = GetLastError();
        if (res || error == ERROR_IO_PENDING) {
            op.wait_interruptible(interruptor);
            error = op.error;
        } else {
            op.set_result(0, error);
        }
        ret = op.nb_bytes;
    }
    if (error == ERROR_BROKEN_PIPE) {
        return 0;
    }
    if (error != NO_ERROR) {
        logWRN("ReadFile failed: %s", winerr_string(error).c_str());
        set_errno(EIO);
        return -1;
    }
    return ret;
}

int64_t socket_stream_t::write(const void *buf, int64_t count) {
    DWORD ret;
    DWORD error;
    if (event_watcher == nullptr) {
        BOOL res = WriteFile(fd, buf, count, &ret, nullptr);
        error = GetLastError();
    } else {
        overlapped_operation_t op(event_watcher);
        BOOL res = WriteFile(fd, buf, count, nullptr, &op.overlapped);
        error = GetLastError();
        if (res || error == ERROR_IO_PENDING) {
            op.wait_interruptible(interruptor);
            error = op.error;
        } else {
            op.set_result(0, error);
        }
        ret = op.nb_bytes;
    }
    if (error != NO_ERROR) {
        logWRN("WriteFile failed: %s", winerr_string(error).c_str());
        set_errno(EIO);
        return -1;
    }
    return ret;
}

void socket_stream_t::wait_for_pipe_client(signal_t *interruptor) {
    rassert(event_watcher != nullptr);
    overlapped_operation_t op(event_watcher);
    // TODO WINDOWS: msdn claim that the overlapped must contain
    // a valid event handle, but it seems to work without one
    BOOL res = ConnectNamedPipe(fd, &op.overlapped);
    DWORD error = GetLastError();
    if (res || error == ERROR_PIPE_CONNECTED) {
        return;
    }
    if (error == ERROR_IO_PENDING) {
        op.wait_interruptible(interruptor);
        error = op.error;
    } else {
        op.set_result(0, error);
    }
    if (error) {
        crash("ConnectNamedPipe failed: %s", winerr_string(error).c_str());
    }
}

#else

#include "containers/archive/socket_stream.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

#include "arch/fd_send_recv.hpp"
#include "arch/runtime/event_queue.hpp" // format_poll_event
#include "concurrency/interruptor.hpp"
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
    guarantee(!interruptor, "blocking_fd_watcher_t doesn't support interruption");
    return true;
}

bool blocking_fd_watcher_t::wait_for_write(signal_t *interruptor) {
    guarantee(write_open_);
    // blocking IO and interruption don't go together nicely
    guarantee(!interruptor, "blocking_fd_watcher_t doesn't support interruption");
    return true;
}

void blocking_fd_watcher_t::init_callback(UNUSED linux_event_callback_t *cb) {}


// -------------------- linux_event_fd_watcher_t --------------------
linux_event_fd_watcher_t::linux_event_fd_watcher_t(fd_t fd)
    : io_in_progress_(false),
      event_callback_(nullptr),
      event_watcher_(fd, this)
{
    int res = fcntl(fd, F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make fd non-blocking.");
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
    event_watcher_.stop_watching_for_errors();
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
    if (interruptor) {
        waiter.add(interruptor);
    }
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
    if (interruptor) {
        waiter.add(interruptor);
    }
    waiter.wait_lazily_unordered();

    guarantee(io_in_progress_);
    io_in_progress_ = false;

    if (interruptor && interruptor->is_pulsed())
        throw interrupted_exc_t();

    return is_write_open();
}

linux_event_fd_watcher_t::~linux_event_fd_watcher_t() {
    assert_thread();
}


// -------------------- socket_stream_t --------------------
socket_stream_t::socket_stream_t(fd_t fd)
    : fd_(fd),
      fd_watcher_(make_scoped<linux_event_fd_watcher_t>(fd)),
      interruptor(NULL)
{
    guarantee(fd != INVALID_FD);
    fd_watcher_->init_callback(this);
}

socket_stream_t::socket_stream_t(fd_t fd, scoped_ptr_t<fd_watcher_t> &&watcher)
    : fd_(fd),
      fd_watcher_(std::move(watcher)),
      interruptor(NULL)
{
    guarantee(fd != INVALID_FD);
    fd_watcher_->init_callback(this);
}

socket_stream_t::~socket_stream_t() {
    assert_thread();
}

int64_t socket_stream_t::read(void *buf, int64_t size) {
    assert_thread();
    guarantee(size > 0);

    if (!check_can_read()) {
        return -1;
    }

    char *bufp = static_cast<char *>(buf);
    for (;;) {
        rassert(is_read_open());

        ssize_t res = ::read(fd_, bufp, size);

        if (res > 0) {
            // We read some data.
            guarantee(res <= size);
            return res;

        } else if (res == -1 && get_errno() == EINTR) {
            // Go around loop & retry system call.

        } else if (res == -1 && (get_errno() == EAGAIN || get_errno() == EWOULDBLOCK)) {
            // Wait until we can read, or we shut down, or we're interrupted.
            if (!wait_for_read())
                return -1;      // we shut down.
            // Go around the loop to try to read again

        } else {
            // EOF or error
            if (res == -1) {
                if (get_errno() != EPIPE && get_errno() != ECONNRESET && get_errno() != ENOTCONN) {
                    // Unexpected error (not just "we closed").
                    logERR("Could not read from socket: %s", errno_string(get_errno()).c_str());
                }
            } else {
                guarantee(res == 0); // sanity
            }

            shutdown_read();
            return res;
        }
    }
}

int64_t socket_stream_t::write(const void *buf, int64_t size) {
    assert_thread();
    guarantee(size > 0);

    if (!check_can_write())
        return -1;

    int64_t orig_size = size;
    const char *bufp = static_cast<const char *>(buf);
    while (size > 0) {
        rassert(is_write_open());

        ssize_t res = ::write(fd_, bufp, size);

        if (res > 0) {
            // We wrote something!
            guarantee(res <= size);
            bufp += res;
            size -= res;
            // Go around the loop; keep writing until done.

        } else if (res == -1 && get_errno() == EINTR) {
            // Go around the loop & retry system call

        } else if (res == -1 && (get_errno() == EAGAIN || get_errno() == EWOULDBLOCK)) {
            // Wait until we can write, or we shut down, or we're interrupted.
            if (!wait_for_write())
                return -1;      // we shut down.
            // Go around the loop and write some more.

        } else {
            if (res == -1) {
                if (get_errno() != EPIPE && get_errno() != ENOTCONN && get_errno() != ECONNRESET) {
                    // Unexpected error (not just "we closed")
                    logERR("Could not write to socket: %s", errno_string(get_errno()).c_str());
                }
            } else {
                logERR("Didn't expect write() to return %zd.", res);
            }

            shutdown_write();
            return -1;
        }
    }

    return orig_size;
}

void socket_stream_t::shutdown_read() {
    assert_thread();

    int res = ::shutdown(fd_, SHUT_RD);
    if (res != 0 && get_errno() != ENOTCONN) {
        logERR("Could not shutdown socket for reading: %s", errno_string(get_errno()).c_str());
    }

    if (fd_watcher_->is_read_open())
        fd_watcher_->on_shutdown_read();
}

void socket_stream_t::shutdown_write() {
    assert_thread();

    int res = ::shutdown(fd_, SHUT_WR);
    if (res != 0 && get_errno() != ENOTCONN) {
        logERR("Could not shutdown socket for writing: %s", errno_string(get_errno()).c_str());
    }

    if (fd_watcher_->is_write_open())
        fd_watcher_->on_shutdown_write();
}

bool socket_stream_t::wait_for_read() {
    interrupted_exc_t saved_exception;
    try {
        return fd_watcher_->wait_for_read(interruptor);
    } catch (const interrupted_exc_t &ex) {
        // Don't shutdown_read here, as is it possible that it may cause a coroutine
        //  switch, which we are not allowed to do in a catch statement
        saved_exception = ex;
    }
    shutdown_read();
    throw saved_exception;
}

bool socket_stream_t::wait_for_write() {
    interrupted_exc_t saved_exception;
    try {
        return fd_watcher_->wait_for_write(interruptor);
    } catch (const interrupted_exc_t &ex) {
        // Don't shutdown_write here, as is it possible that it may cause a coroutine
        //  switch, which we are not allowed to do in a catch statement
        saved_exception = ex;
    }
    shutdown_write();
    throw saved_exception;
}

bool socket_stream_t::check_can_read() {
    if (!is_read_open()) return false;
    if (interruptor && interruptor->is_pulsed()) throw interrupted_exc_t();
    return true;
}

bool socket_stream_t::check_can_write() {
    if (!is_write_open()) return false;
    if (interruptor && interruptor->is_pulsed()) throw interrupted_exc_t();
    return true;
}

void socket_stream_t::do_on_event(int /*events*/) {
    // The default behavior is to do nothing.
}

void socket_stream_t::on_event(int events) {
    assert_thread();
    do_on_event(events);

    /* This is called by linux_event_watcher_t when error events occur. Ordinary
    poll_event_in/poll_event_out events are not sent through this function. */

    // Shut down.
    if (fd_watcher_->is_write_open()) shutdown_write();
    if (fd_watcher_->is_read_open()) shutdown_read();
}

#endif
