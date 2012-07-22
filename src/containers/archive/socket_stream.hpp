#ifndef CONTAINERS_ARCHIVE_SOCKET_STREAM_HPP_
#define CONTAINERS_ARCHIVE_SOCKET_STREAM_HPP_

#include "arch/io/event_watcher.hpp" // linux_event_watcher_t
#include "arch/io/io_utils.hpp"      // scoped_fd_t
#include "concurrency/cond_var.hpp"
#include "containers/archive/archive.hpp"
#include "containers/scoped.hpp"
#include "errors.hpp"

/* fd_watcher_t exists to factor the problem of "how to wait for I/O on an fd"
 * out of fd_stream_t. The best answer, if available, is to use a
 * linux_event_watcher_t to do nonblocking I/O. However, we may want to use
 * classes descended from fd_stream_t in contexts where the infrastructure
 * needed for linux_event_watcher_t is not in place (eg. external JS processes).
 */

class fd_watcher_t : public home_thread_mixin_t {
  public:
    fd_watcher_t() {}
    virtual ~fd_watcher_t() {}

    virtual void init_callback(linux_event_callback_t *cb) = 0;

    virtual bool is_read_open() = 0;
    virtual bool is_write_open() = 0;
    virtual void on_shutdown_read() = 0;
    virtual void on_shutdown_write() = 0;

    // wait_for_{read,write} wait for either an opportunity to read/write, or
    // for us to shutdown for reading/writing, whichever happens first. It
    // returns true if we can read/write and false if we have shut down (in
    // which case on_shutdown_{read,write} has already been called).
    virtual MUST_USE bool wait_for_read() = 0;
    virtual MUST_USE bool wait_for_write() = 0;

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
    virtual MUST_USE bool wait_for_read();
    virtual MUST_USE bool wait_for_write();
    virtual void init_callback(linux_event_callback_t *cb);

  private:
    bool read_open_, write_open_;
};

/* linux_event_fd_watcher_t uses a linux_event_watcher to wait for IO, and makes
 * its corresponding fd use non-blocking I/O.
 */
class linux_event_fd_watcher_t :
    public fd_watcher_t, private linux_event_callback_t
{
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
    virtual MUST_USE bool wait_for_read();
    virtual MUST_USE bool wait_for_write();

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
    public read_stream_t, public write_stream_t,
    private linux_event_callback_t
{
  public:
    // calls `fd->release()` & takes ownership of its fd.
    // takes ownership of watcher (if supplied); by default constructs a
    // linux_event_fd_watcher_t.
    explicit socket_stream_t(scoped_fd_t *fd, fd_watcher_t *watcher = NULL);
    virtual ~socket_stream_t();

    // {read,write}_stream_t functions
    // Returns number of bytes read or 0 upon EOF, -1 upon error.
    virtual MUST_USE int64_t read(void *p, int64_t n);
    // Returns n, or -1 upon error. Blocks until all bytes are written.
    virtual int64_t write(const void *p, int64_t n);

    void assert_thread() { fd_watcher_->assert_thread(); }

    bool is_read_open() { return fd_watcher_->is_read_open(); }
    bool is_write_open() { return fd_watcher_->is_write_open(); }

  protected:
    void shutdown_read();
    void shutdown_write();

    virtual void on_event(int events); // for linux_callback_t

    // Member fields
  protected:
    scoped_fd_t fd_;
    scoped_ptr_t<fd_watcher_t> fd_watcher_;

  private:
    DISABLE_COPYING(socket_stream_t);
};

class unix_socket_stream_t : public socket_stream_t {
  public:
    explicit unix_socket_stream_t(scoped_fd_t *fd, fd_watcher_t *watcher = NULL);

    // WARNING. Sending large numbers of fds at once is contraindicated. See
    // arch/fd_send_recv.hpp. TODO(rntz): fix this.

    // Sends open file descriptors. Must be matched by a call to recv_fd{s,} on
    // the other end, or weird shit could happen.
    //
    // Returns -1 on error, 0 on success. Blocks until all fds are written.
    int send_fds(size_t num_fds, fd_t *fds);
    int send_fd(fd_t fd);

    // Receives open file descriptors. Must only be called to match a call to
    // write_fd{s,} on the other end; otherwise undefined behavior could result!
    // Blocks until all fds are received.
    MUST_USE archive_result_t recv_fds(size_t num_fds, fd_t *fds);
    MUST_USE archive_result_t recv_fd(fd_t *fd);
};

#endif  // CONTAINERS_ARCHIVE_SOCKET_STREAM_HPP_
