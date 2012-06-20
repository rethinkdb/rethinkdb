#ifndef CONTAINERS_ARCHIVE_FD_STREAM_HPP_
#define CONTAINERS_ARCHIVE_FD_STREAM_HPP_

#include "arch/io/event_watcher.hpp" // linux_event_watcher_t
#include "arch/io/io_utils.hpp"      // scoped_fd_t
#include "concurrency/cond_var.hpp"
#include "containers/archive/archive.hpp"
#include "errors.hpp"
#include "utils.hpp"                 // home_thread_mixin_t

#include <boost/scoped_ptr.hpp>

// This is very similar to linux_tcp_conn_t.
class fd_stream_t :
    public read_stream_t, public write_stream_t,
    public home_thread_mixin_t,
    private linux_event_callback_t
{
  public:
    // takes ownership of fd
    fd_stream_t(int fd);
    virtual ~fd_stream_t();

    // {read,write}_stream_t functions
    // Returns number of bytes read or 0 upon EOF, -1 upon error.
    MUST_USE int64_t read(void *p, int64_t n);
    // Returns n, or -1 upon error. Blocks until all bytes are written.
    int64_t write(const void *p, int64_t n);

  protected:
    bool is_read_open();
    bool is_write_open();

    // wait_for_{read,write} wait for either an opportunity to read/write, or
    // for us to shutdown for reading/writing, whichever happens first. It
    // returns true if we can read/write and false if we have shut down (in
    // which case on_shutdown_{read,write} has already been called).
    MUST_USE bool wait_for_read();
    MUST_USE bool wait_for_write();

    void shutdown_read();
    void shutdown_write();

    // Must be overriden to determine what we do with errors.
    virtual void on_read_error(int errsv) = 0;
    virtual void on_write_error(int errsv) = 0;

    // Must be overriden to determine how we perform a {read,write} shutdown.
    virtual void do_shutdown_read() = 0;
    virtual void do_shutdown_write() = 0;

  private:
    void on_shutdown_read();
    void on_shutdown_write();
    void on_event(int events);  // for linux_event_callback_t

    // Member fields
  protected:
    scoped_fd_t fd_;
    boost::scoped_ptr<linux_event_watcher_t> event_watcher_;

  private:
    /* True if there is a pending read or write. Used to check that we are used
     * in a single-threaded fashion. */
    bool io_in_progress_;

    /* These are pulsed if and only if the read/write end of the connection has been closed. */
    cond_t read_closed_, write_closed_;

  private:
    DISABLE_COPYING(fd_stream_t);
};

class socket_stream_t :
    public fd_stream_t
{
  public:
    socket_stream_t(int fd);

  protected:
    virtual void on_read_error(int errno_);
    virtual void on_write_error(int errno_);
    virtual void do_shutdown_read();
    virtual void do_shutdown_write();

  private:
    DISABLE_COPYING(socket_stream_t);
};

class unix_socket_stream_t :
    public socket_stream_t
{
  public:
    unix_socket_stream_t(int fd);

    // Sends open file descriptors. Must be matched by a call to recv_fd{s,} on
    // the other end, or weird shit could happen.
    //
    // Returns -1 on error, 0 on success. Blocks until all fds are written.
    int send_fds(int64_t num_fds, int *fds);
    int send_fd(int fd);

    // Receives open file descriptors. Must only be called to match a call to
    // write_fd{s,} on the other end; otherwise undefined behavior could result!
    // Blocks until all fds are received.
    MUST_USE archive_result_t recv_fds(int64_t num_fds, int *fds);
    MUST_USE archive_result_t recv_fd(int *fd);

  private:
    DISABLE_COPYING(unix_socket_stream_t);
};

#endif  // CONTAINERS_ARCHIVE_FD_STREAM_HPP_
