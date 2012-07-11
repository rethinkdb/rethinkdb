#ifndef JSPROC_SPAWNER_HPP_
#define JSPROC_SPAWNER_HPP_

#include <sys/types.h>                    // pid_t

#include "arch/runtime/runtime_utils.hpp" // fd_t
#include "concurrency/mutex.hpp"
#include "containers/archive/socket_stream.hpp"

namespace jsproc {

// A handle to the spawner process. Must only be used from the thread it is
// created on.
class spawner_t :
    public home_thread_mixin_t
{
  public:
    class info_t {
        friend class spawner_t;
        pid_t pid;
        fd_t socket;
    };

    // Must be called within a thread pool, exactly once.
    explicit spawner_t(const info_t &info);
    ~spawner_t();

    // Called to fork off the spawner child process. Must be called *OUTSIDE* of
    // a thread pool (ie. before run_in_thread_pool).
    //
    // On success, returns the pid of the spawner process and initializes
    // `*info` in the parent process, and does not return in the child (instead
    // it runs the spawner).
    //
    // On failure, returns -1 in the parent process (the child is dead or never
    // existed).
    //
    // NOTE: It is up to the caller to establish a SIGCHLD handler or some other
    // way of dealing with the spawner dying. The spawner process itself will
    // die as soon as it notices the other end of its socket has closed.
    static pid_t create(info_t *info);

    // Creates a socket pair and spawns a process with access to one end of it.
    // On success, returns the pid of the child process and initializes
    // `*socket` to our end of the socket. The child process deserializes one
    // job from the socket, runs it, then exits. See job.hpp.
    //
    // Returns -1 on error.
    pid_t spawn_process(fd_t *socket);

  private:
    // Called by create() in the spawner process.
    static void exec_spawner(fd_t sockfd);

    // Called by spawn_process() in the worker process.
    static void exec_worker(fd_t sockfd);

  private:
    pid_t pid_;
    mutex_t mutex_;             // linearizes access to socket_
    unix_socket_stream_t socket_;
};

} // namespace jsproc

#endif // JSPROC_SPAWNER_HPP_
