// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_SOCKET_STREAM_HPP_
#define CONTAINERS_ARCHIVE_SOCKET_STREAM_HPP_

#ifdef _WIN32

#include "concurrency/signal.hpp"
#include "containers/archive/archive.hpp"
#include "containers/scoped.hpp"
#include "arch/io/event_watcher.hpp"
#include "arch/runtime/runtime.hpp"

class blocking_fd_watcher_t { };

class socket_stream_t :
    public read_stream_t,
    public write_stream_t {
public:
    socket_stream_t(fd_t fd_, scoped_ptr_t<blocking_fd_watcher_t>)
        : fd(fd_),
          interruptor(nullptr),
          event_watcher(nullptr) { }
    socket_stream_t(fd_t fd_, windows_event_watcher_t *ew)
        : fd(fd_),
          event_watcher(ew),
          interruptor(nullptr) {
        rassert(ew != nullptr);
        rassert(ew->current_thread() == get_thread_id());
    }

    socket_stream_t(const socket_stream_t &) = default;

    int64_t read(void *buf, int64_t count);
    int64_t write(const void *buf, int64_t count);
    void wait_for_pipe_client(signal_t *interruptor);

    void set_interruptor(signal_t *_interruptor) { interruptor = _interruptor; }

private:
    fd_t fd;
    signal_t *interruptor;
    windows_event_watcher_t *event_watcher;
};

#else

#include "concurrency/signal.hpp"
#include "containers/archive/archive.hpp"

#include "arch/io/event_watcher.hpp"
#include "arch/io/io_utils.hpp"
#include "concurrency/cond_var.hpp"
#include "containers/scoped.hpp"
#include "errors.hpp"

/* fd_watcher_t exists to factor the problem of "how to wait for I/O on an fd"
 * out of fd_stream_t. The best answer, if available, is to use a
 * linux_event_watcher_t to do nonblocking I/O. However, we may want to use
 * classes descended from fd_stream_t in contexts where the infrastructure
 * needed for linux_event_watcher_t is not in place (eg. external JS processes).
 */

class fd_watcher_t : public home_thread_mixin_debug_only_t {
public:
    fd_watcher_t() {}
    virtual ~fd_watcher_t() {}

    virtual void init_callback(linux_event_callback_t *cb) = 0;

    virtual bool is_read_open() = 0;
    virtual bool is_write_open() = 0;
    virtual void on_shutdown_read() = 0;
    virtual void on_shutdown_write() = 0;

    // wait_for_{read,write} wait for either an opportunity to read/write, or
    // for us to shutdown for reading/writing, or for `interruptor` (if
    // non-NULL) to be pulsed, whichever happens first. It returns true if we
    // can read/write and false if we have shut down (in which case
    // on_shutdown_{read,write} has already been called).
    //
    // They raise interrupted_exc_t if `interruptor` is pulsed.
    virtual MUST_USE bool wait_for_read(signal_t *interruptor) = 0;
    virtual MUST_USE bool wait_for_write(signal_t *interruptor) = 0;

private:
    DISABLE_COPYING(fd_watcher_t);
};

/* blocking_fd_watcher_t is the simplest fd_watcher_t: it doesn't wait for IO.
 */
class blocking_fd_watcher_t : public fd_watcher_t {
public:
    blocking_fd_watcher_t();

    virtual bool is_read_open();
    virtual bool is_write_open();
    virtual void on_shutdown_read();
    virtual void on_shutdown_write();
    virtual MUST_USE bool wait_for_read(signal_t *interruptor);
    virtual MUST_USE bool wait_for_write(signal_t *interruptor);
    virtual void init_callback(linux_event_callback_t *cb);

private:
    bool read_open_, write_open_;
};

/* linux_event_fd_watcher_t uses a linux_event_watcher to wait for IO, and makes
 * its corresponding fd use non-blocking I/O.
 */
class linux_event_fd_watcher_t :
    public fd_watcher_t, private linux_event_callback_t {
public:
    // does not take ownership of fd
    explicit linux_event_fd_watcher_t(fd_t fd);
    virtual ~linux_event_fd_watcher_t();

    virtual void init_callback(linux_event_callback_t *cb);
    virtual void on_event(int events);

    virtual bool is_read_open();
    virtual bool is_write_open();
    virtual void on_shutdown_read();
    virtual void on_shutdown_write();
    virtual MUST_USE bool wait_for_read(signal_t *interruptor);
    virtual MUST_USE bool wait_for_write(signal_t *interruptor);

private:
    // True iff there is a waiting read/write operation. Used to ensure that we
    // are used in a single-threaded fashion.
    bool io_in_progress_;

    /* These are pulsed if and only if the read/write end of the connection has
     * been closed. */
    cond_t read_closed_, write_closed_;

    // We forward to this callback on error events.
    linux_event_callback_t *event_callback_;

    // The linux_event_watcher that we use to wait for IO events.
    linux_event_watcher_t event_watcher_;

    DISABLE_COPYING(linux_event_fd_watcher_t);
};

class socket_stream_t :
    public read_stream_t,
    public write_stream_t,
    private linux_event_callback_t {
public:
    explicit socket_stream_t(fd_t fd);
    explicit socket_stream_t(fd_t fd, scoped_ptr_t<fd_watcher_t> &&watcher);
    virtual ~socket_stream_t();

    // interruptible {read,write}_stream_t functions
    virtual MUST_USE int64_t read(void *p, int64_t n);
    virtual int64_t write(const void *p, int64_t n);

    void set_interruptor(signal_t *_interruptor) { interruptor = _interruptor; }

    void assert_thread() { fd_watcher_->assert_thread(); }

    bool is_read_open() { return fd_watcher_->is_read_open(); }
    bool is_write_open() { return fd_watcher_->is_write_open(); }

private:
    void shutdown_read();
    void shutdown_write();

    // Returns false if we are closed for {read,write}.
    // Raises interrupted_exc_t if we are open but interruptor is pulsed.
    // Returns true otherwise.
    bool check_can_read();
    bool check_can_write();

    // Wrappers for fd_watcher_->wait_for_{read,write} that shut us down if
    // interrupted.
    bool wait_for_read();
    bool wait_for_write();

    // Member fields
    // For subclasses to override on_event behavior.  Is evaluated as the first
    // thing done in on_event.
    virtual void do_on_event(int events);

    fd_t fd_;
    scoped_ptr_t<fd_watcher_t> fd_watcher_;
    signal_t *interruptor;

    void on_event(int events); // for linux_callback_t

    DISABLE_COPYING(socket_stream_t);
};

#endif // _WIN32

#endif  // CONTAINERS_ARCHIVE_SOCKET_STREAM_HPP_
