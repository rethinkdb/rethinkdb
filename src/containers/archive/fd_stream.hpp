#ifndef CONTAINERS_ARCHIVE_FD_STREAM_HPP_
#define CONTAINERS_ARCHIVE_FD_STREAM_HPP_

#include <boost/scoped_ptr.hpp>

#include "arch/io/event_watcher.hpp" // linux_event_watcher_t
#include "arch/io/io_utils.hpp"      // scoped_fd_t
#include "concurrency/cond_var.hpp"
#include "containers/archive/archive.hpp"
#include "errors.hpp"
#include "utils.hpp"                 // home_thread_mixin_t

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
    void shutdown_read();
    void shutdown_write();

    // Overridable to change what we do on errors. By default, we log them all
    // as errors.
    virtual void on_read_error(int errno_);
    virtual void on_write_error(int errno_);

    // Overridable to add behavior to shutdown_{read,write}(). By default, we do
    // nothing except note that the read/write direction has closed.
    virtual void do_shutdown_read();
    virtual void do_shutdown_write();

  private:
    void on_shutdown_read();
    void on_shutdown_write();
    void on_event(int events);  // for linux_event_callback_t

    // Member fields
  protected:
    scoped_fd_t fd_;
    boost::scoped_ptr<linux_event_watcher_t> event_watcher_;

  private:
#ifndef NDEBUG
    /* True if there is a pending read or write */
    bool io_in_progress_;
#endif

    /* These are pulsed if and only if the read/write end of the connection has been closed. */
    cond_t read_closed_, write_closed_;

  private:
    DISABLE_COPYING(fd_stream_t);
};

class socket_stream_t :
    public fd_stream_t
{
    socket_stream_t(int fd);

  protected:
    virtual void on_read_error(int errno_);
    virtual void on_write_error(int errno_);
    virtual void do_shutdown_read();
    virtual void do_shutdown_write();

  private:
    DISABLE_COPYING(socket_stream_t);
};

#endif
