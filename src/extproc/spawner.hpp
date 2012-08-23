#ifndef EXTPROC_SPAWNER_HPP_
#define EXTPROC_SPAWNER_HPP_

#include <sys/types.h>

#include "arch/runtime/runtime_utils.hpp"
#include "concurrency/mutex.hpp"
#include "containers/archive/socket_stream.hpp"

namespace extproc {

// A handle to the spawner process. Must only be used from the thread it is
// created on.
class spawner_t :
    public home_thread_mixin_t
{
  public:
    class info_t {
        friend class spawner_t;
        pid_t pid;
        scoped_fd_t socket;

        DISABLE_COPYING(info_t);

      public:
        info_t() {}
        ~info_t() {}
    };

    // Must be called within a thread pool, exactly once.
    explicit spawner_t(info_t *info);
    ~spawner_t();

    // Called to fork off the spawner child process. Must be called *OUTSIDE* of
    // a thread pool (ie. before run_in_thread_pool).
    static void create(info_t *info);

    // Creates a socket pair and spawns a process with access to one end of it.
    // On success, returns the pid of the child process and initializes
    // `*socket` to our end of the socket. The child process deserializes one
    // job from the socket, runs it, then exits. See job.hpp.
    //
    // Returns -1 on error.
    pid_t spawn_process(scoped_fd_t *socket);

  private:
    static void exec_spawner(fd_t socket) NORETURN;
    static void exec_worker(fd_t socket) NORETURN;

  private:
    pid_t pid_;
    mutex_t mutex_;             // linearizes access to socket_
    unix_socket_stream_t socket_;
};

} // namespace extproc

#endif // EXTPROC_SPAWNER_HPP_
