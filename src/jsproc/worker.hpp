#ifndef JSPROC_WORKER_HPP_
#define JSPROC_WORKER_HPP_

#include "containers/archive/fd_stream.hpp"
#include "jsproc/job.hpp"

namespace jsproc {

// Called in a worker process to do the heavy lifting. Never returns.
void exec_worker(int socket_fd);

} // namespace jsproc

#endif // JSPROC_WORKER_HPP_
