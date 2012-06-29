#ifndef JSPROC_WORKER_PROC_HPP_
#define JSPROC_WORKER_PROC_HPP_

namespace jsproc {

// Called in a worker process to do the heavy lifting. Never returns.
void exec_worker(int socket_fd);

} // namespace jsproc

#endif // JSPROC_WORKER_PROC_HPP_
