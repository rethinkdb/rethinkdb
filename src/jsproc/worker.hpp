#ifndef JSPROC_WORKER_HPP_
#define JSPROC_WORKER_HPP_

#include "containers/archive/fd_stream.hpp"
#include "jsproc/job.hpp"

namespace jsproc {

// A handle to a worker process. POD.
class worker_t {
  public:
    pid_t pid;
    int fd;

    // Static utility method called on the supervisor side.
    static int spawn(worker_t *proc);

  private:
    // Static utility method called by spawn() on the worker-process side.
    static int run_worker(int sockfd);
};

} // namespace jsproc

#endif // JSPROC_WORKER_HPP_
