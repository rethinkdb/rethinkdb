#ifndef JSPROC_SUPERVISOR_PROC_HPP_
#define JSPROC_SUPERVISOR_PROC_HPP_

#include <deque>                // std::deque
#include <signal.h>             // siginfo_t
#include <unistd.h>             // pid_t

#include "containers/intrusive_list.hpp"

namespace jsproc {

class supervisor_proc_t {
  public:
    supervisor_proc_t(pid_t child, int sockfd);
    ~supervisor_proc_t();

    // Used for spawning new workers. See exec_supervisor(), spawn().
    struct exec_worker_exc_t {
        explicit exec_worker_exc_t(int _fd) : fd(_fd) {}
        int fd;
    };

    struct worker_t : public intrusive_list_node_t<worker_t> {
        worker_t(pid_t _pid, int _fd) : pid(_pid), fd(_fd) {}
        ~worker_t();
        pid_t pid; int fd;
    };

    // Never returns.
    void run() THROWS_ONLY(exec_worker_exc_t);

  private:
    // The rethinkdb engine child process.
    pid_t engine_pid_;
    int engine_fd_;

    // Worker processes.
    typedef intrusive_list_t<worker_t> workers_t;
    workers_t idle_workers_, busy_workers_;

    // Unfulfilled requests from the engine.
    std::deque<int> requests_;

  private:
    void event_loop();

    void wait_for_events();
    void init_fdsets(int *nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds);
    void get_new_idle_procs();
    void handle_requests();
    void enforce_bounds();

    // Tries to send a request to an idle worker. Returns false on failure.
    // `*worker` must point to a worker in idle_workers_. On return, it has been
    // advanced to the next idle worker (or NULL if we have finished the list).
    bool send_request(worker_t **worker, int reqfd);

    // Called by signal handler when a child of ours dies.
    void on_sigchild(siginfo_t *info);

    void spawn_workers(int num);
    worker_t *create_worker() THROWS_ONLY(exec_worker_exc_t);
    void end_worker(workers_t *list, worker_t *child);
    void end_workers();

    // Shuts down all worker processes and waits for the rethindkb engine to
    // die, then propagates its exit code.
    void shut_down();

    void panicf(const char *fmt, ...) __attribute__((format (printf, 2, 3)));
    void panic();

    static void close_workers(workers_t *workers);
};

} // namespace jsproc

#endif // JSPROC_SUPERVISOR_PROC_HPP_
