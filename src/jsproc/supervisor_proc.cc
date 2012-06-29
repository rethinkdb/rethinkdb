#include "jsproc/supervisor_proc.hpp"

#include <algorithm>            // std::{min,max}
#include <cstdio>               // fprintf() & co
#include <cstring>              // strerror()
#include <fcntl.h>              // fcntl()
#include <signal.h>             // kill(), SIGKILL
#include <stdarg.h>             // va_start() & co
#include <sys/select.h>         // select()
#include <sys/socket.h>         // socketpair()
#include <sys/time.h>           // select()
#include <sys/types.h>          // wait(), select()
#include <sys/un.h>             // AF_UNIX
#include <sys/wait.h>           // wait()
#include <unistd.h>             // select()

#include "arch/fd_send_recv.hpp"
#include "containers/intrusive_list.hpp"
#include "logger.hpp"

// TODO(rntz): a way to kill off unneeded worker processes if we have too many.
// Currently the number of JS procs we have grows monotonically until it hits
// MAX_JS_PROCS.
//
// Currently MIN_JS_PROCS == MAX_JS_PROCS so this isn't an issue. Once we need a
// more flexible policy for how many js processes to run, revisit this issue.

// Maximum number of open requests for fds that the supervisor will accept.
#define MAX_FD_REQUESTS         1

// The supervisor will attempt to ensure that there are always at least this
// many total javascript processes. This is therefore the size of the initial
// worker pool. Must be at least MIN_IDLE_JS_PROCS.
#define MIN_JS_PROCS            8

// The supervisor will never spawn more than this many JS processes, period.
// This takes priority over MIN_IDLE_JS_PROCS.
#define MAX_JS_PROCS            8

// The supervisor will attempt to ensure that there are always at least this
// many idle javascript processes.
#define MIN_IDLE_JS_PROCS       0

namespace jsproc {

#define PANIC(fmt, ...) panicf("supervisor: panicking: " fmt "\n", ## __VA_ARGS__)

supervisor_proc_t::worker_t::~worker_t() {
    guarantee_err(0 == close(fd), "could not close fd\n");
}

supervisor_proc_t::supervisor_proc_t(pid_t child, int sockfd) :
    engine_pid_(child), engine_fd_(sockfd)
{
    guarantee(0 == fcntl(engine_fd_, F_SETFL, O_NONBLOCK),
              "could not make socket non-blocking");
}

supervisor_proc_t::~supervisor_proc_t() {
    // Close all our open fds.
    guarantee_err(0 == close(engine_fd_), "could not close fd");
    close_workers(&idle_workers_);
    close_workers(&busy_workers_);
}

void supervisor_proc_t::close_workers(workers_t *workers) {
    worker_t *w;
    while ((w = workers->head())) {
        workers->remove(w);
        delete w;
    }
}

void supervisor_proc_t::run() THROWS_ONLY(exec_worker_exc_t) {
    // We accept all signals except SIGCHLD. We accept SIGCHLD only in certain
    // places, to avoid concurrency problems.
    sigset_t childmask;
    guarantee_err(0 == sigemptyset(&childmask), "Could not create empty signal set");
    guarantee_err(0 == sigaddset(&childmask, SIGCHLD), "Could not add SIGCHLD to signal set");
    guarantee(0 == sigprocmask(SIG_SETMASK, &childmask, NULL), "Could not set signal mask");

    // Set up signal handler for SIGCHLD, using RAII to ensure that it gets
    // unset when this function exits.
    static supervisor_proc_t *supervisor_proc = this;

    struct sa_setter_t {
        static void signal_handler(int signo, siginfo_t *info, UNUSED void *ucontext) {
            guarantee(signo == SIGCHLD);
            guarantee(supervisor_proc);
            supervisor_proc->on_sigchild(info);
        }

        sa_setter_t() {
            struct sigaction sa;
            guarantee(0 == sigemptyset(&sa.sa_mask), "Could not create empty signal set");
            sa.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;
            sa.sa_sigaction = signal_handler;
            guarantee_err(0 == sigaction(SIGCHLD, &sa, NULL), "Could not set signal handler");
        }

        ~sa_setter_t() {
            struct sigaction sa;
            sa.sa_handler = SIG_DFL;
            guarantee(0 == sigemptyset(&sa.sa_mask), "Could not create empty signal set");
            sa.sa_flags = 0;
            guarantee_err(0 == sigaction(SIGCHLD, &sa, NULL), "Could not set signal handler");
        }
    } sa_setter;

    // TODO (rntz): make SIGTERM, SIGINT handlers that Do The Right Thing.
    // perhaps this means forwarding to the rethinkdb engine?
    // talk to someone about this

    // Fork off initial worker pool.
    guarantee(MIN_IDLE_JS_PROCS <= MIN_JS_PROCS); // sanity
    spawn_workers(MIN_JS_PROCS);

    // Main event loop
    event_loop();
}

void supervisor_proc_t::event_loop() {
    for (;;) {
        wait_for_events();

        // Check whether any busy js processes have sent us a byte, indicating
        // they have become ready.
        get_new_idle_procs();

        // Read requests from the engine, and unload requests (from queue or
        // engine) onto idle procs.
        handle_requests();

        // Spawn new processes as appropriate to ensure our minimum requested
        // idle/total processes, etcetera.
        enforce_bounds();
    }
}

void supervisor_proc_t::wait_for_events() {
    // Set up fd set.
    int nfds;
    fd_set readfds, writefds, exceptfds;
    init_fdsets(&nfds, &readfds, &writefds, &exceptfds);

    // Allow signals to get at us while we're waiting for events. pselect()
    // exists for this purpose, but it's less portable than select() and the
    // atomicity issues it solves aren't a problem here.
    //
    // The reason we unmask SIGCHLD here but not anywhere else is to avoid race
    // conditions: this ensures that the SIGCHLD handler doesn't get run while
    // we're eg. manipulating a process list.
    sigset_t emptymask, oldmask;
    guarantee_err(0 == sigemptyset(&emptymask), "Could not create empty signal set");
    guarantee_err(0 == sigprocmask(SIG_SETMASK, &emptymask, &oldmask), "Could not unblock signals");

    // Wait for events.
    int res = select(nfds, &readfds, &writefds, &exceptfds, NULL);

    // Restore old signal mask; no more SIGCHLD.
    guarantee_err(0 == sigprocmask(SIG_SETMASK, &oldmask, NULL), "Could not block signals");

    if (res == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
        // Not a problem; just loop round again and we'll handle it.
        res = 0;
    }

    guarantee_err(res != -1, "Waiting for select() failed");
}

static inline void addfd(int fd, fd_set *fdset, int *nfds) {
    if (fd+1 > *nfds)
        *nfds = fd+1;
    FD_SET(fd, fdset);
}

void supervisor_proc_t::init_fdsets(int *nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds) {
    FD_ZERO(readfds);
    FD_ZERO(writefds);
    FD_ZERO(exceptfds);

    *nfds = 0;

    // We always watch our busy processes for reading, in case they become idle,
    // which they signal by sending us a single byte.
    for (worker_t *w = busy_workers_.head(); w; w = busy_workers_.next(w)) {
        addfd(w->fd, readfds, nfds);
    }

    // If we have room on the request queue, we watch the engine fd for read, so
    // that it can send us requests.
    if (requests_.size() < MAX_FD_REQUESTS) {
        addfd(engine_fd_, readfds, nfds);
    }

    // If we have pending requests, we watch all idle processes for write.
    if (!requests_.empty()) {
        for (worker_t *w = idle_workers_.head(); w; w = idle_workers_.next(w)) {
            addfd(w->fd, writefds, nfds);
        }
    }
}

void supervisor_proc_t::get_new_idle_procs() {
    // For each busy process, check whether we can read from its fd.
    for (worker_t *worker = busy_workers_.head(), *next; worker; worker = next) {
        // We might remove worker from busy_workers_ later, so we do this now.
        next = busy_workers_.next(worker);

        char c;
        ssize_t res = read(worker->fd, &c, 1);

        if (res == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Nothing to do; keep going.
            continue;
        }

        if (res == 0 || res == -1) {
            // EOF or shut-down. Worker process is no longer active.
            fprintf(stderr, "supervisor: read from child %d failed (%s), killing it\n",
                    worker->pid, strerror(errno));
            end_worker(&busy_workers_, worker);
            continue;
        }

        // Add it to the idle worker list.
        guarantee(res == 1);
        busy_workers_.remove(worker);
        idle_workers_.push_back(worker);
    }
}

void supervisor_proc_t::handle_requests() {
    worker_t *worker = idle_workers_.head();

    // Unload requests from the front of the queue.
    std::deque<int>::iterator fdi = requests_.begin();
    while (fdi != requests_.end() && worker) {
        if (send_request(&worker, *fdi)) {
            // Success! Handle the next request.
            fdi = requests_.erase(fdi);
        }
        // send_request updates `worker` and cleans up for us in case of failure
    }

    // Either we serviced all requests, or we ran out of idle workers which were
    // ready to accept jobs.
    guarantee(requests_.empty() || !worker);

    // Read requests from the engine and either send them to an idle worker or
    // put them in the queue, until the queue is full or there are no more
    // requests.
    while (requests_.size() < MAX_FD_REQUESTS) {
        int fd = 0;
        fd_recv_result_t res = recv_fds(engine_fd_, 1, &fd);
        if (res == FD_RECV_ERROR &&
            (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
            // The engine has no more requests, and that's fine by us.
            break;
        }

        switch (res) {
          case FD_RECV_OK:
            // Try to find a willing worker.
            guarantee(fd);
            while (worker) {
                if (send_request(&worker, fd)) {
                    fd = 0;     // indicates we managed to fulfill the request
                    break;
                }
            }
            if (fd) {
                // Couldn't find a willing worker; push on queue.
                requests_.push_back(fd);
            }
            break;

          case FD_RECV_EOF:
            // When the engine process closes its connection to us, that means
            // it has no further need of us, so we initiate an orderly shutdown.
            shut_down();
            unreachable();

          case FD_RECV_ERROR:
          case FD_RECV_INVALID:
            PANIC("read from rethinkdb engine failed (%s), shutting down",
                  res == FD_RECV_ERROR ? strerror(errno) :
                  res == FD_RECV_EOF ? "EOF received" :
                  "invalid request - PROBABLY A BUG, PLEASE REPORT");

          default: unreachable();
        }
    }
}

bool supervisor_proc_t::send_request(worker_t **workerp, int fd) {
    worker_t *worker = *workerp;
    *workerp = idle_workers_.next(worker);

    if (!send_fds(worker->fd, 1, &fd)) {
        // Success! Move worker to busy worker list.
        idle_workers_.remove(worker);
        busy_workers_.push_back(worker);
        return true;
    }

    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        // Worker isn't ready for us.
    } else {
        // Unknown error. Kill offending worker and update iterator.
        fprintf(stderr, "supervisor: write to child %d failed (%s), killing it\n",
                worker->pid, strerror(errno));
        end_worker(&idle_workers_, worker);
    }
    return false;
}

void supervisor_proc_t::enforce_bounds() {
    // NB. we count outstanding requests against the number of idle processes we
    // have for checking MIN_IDLE_JS_PROCS.
    int total_procs = idle_workers_.size() + busy_workers_.size();
    int idle_procs = idle_workers_.size() - requests_.size();
    int i = std::min(MAX_JS_PROCS - total_procs,
                     std::max(MIN_IDLE_JS_PROCS - idle_procs,
                              MIN_JS_PROCS - total_procs));

    if (i > 0) {
        spawn_workers(i);
    }
}

void supervisor_proc_t::on_sigchild(siginfo_t *info) {
    guarantee(info->si_signo == SIGCHLD);

    switch (info->si_code) {
      case CLD_EXITED: case CLD_KILLED: case CLD_DUMPED: break; // normal

      case SI_USER: case SI_TKILL:
        fprintf(stderr, "supervisor: Received SIGCHLD from kill(2), tkill(2), "
                "or tgkill(2); ignoring.\n");
        return;

      default:
        PANIC("received SIGCHLD for unknown reason");
    }

    pid_t pid = info->si_pid;

    if (pid == engine_pid_) {
        shut_down();

    } else {
        // Find the worker process that died.
        for (worker_t *w = busy_workers_.head(); w; w = busy_workers_.next(w))
            if (pid == w->pid) {
                busy_workers_.remove(w);
                delete w;
                goto found;
            }

        for (worker_t *w = idle_workers_.head(); w; w = idle_workers_.next(w))
            if (pid == w->pid) {
                idle_workers_.remove(w);
                delete w;
                goto found;
            }

        // We didn't find it; that just means it's already been removed from our
        // lists by end_worker(). No problem, we wait on it anyway to clean up
        // the zombie process.

      found:
        // wait() for it, to clean up zombies.
        pid_t res = waitpid(pid, NULL, WNOHANG);
        guarantee_err(res != -1, "could not waitpid() on dead child");
        guarantee(res != 0, "SIGCHLD reported child dead, but it's alive?");
        guarantee(pid == res);
    }
}

void supervisor_proc_t::spawn_workers(int num) {
    guarantee(num >= 0);
    for (int i = 0; i < num; ++i) {
        worker_t *worker = create_worker();
        if (worker) idle_workers_.push_back(worker);
    }
}

supervisor_proc_t::worker_t *supervisor_proc_t::create_worker() THROWS_ONLY(exec_worker_exc_t) {
    int fds[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    if (res) {
        perror("could not make socketpair() for worker process");
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        guarantee_err(0 == close(fds[0]), "could not close fd");
        guarantee_err(0 == close(fds[1]), "could not close fd");
        perror("could not fork() worker process");
        return NULL;
    }

    if (!pid) {
        // We're the child.
        guarantee_err(0 == close(fds[0]), "could not close fd");
        throw exec_worker_exc_t(fds[1]);
        unreachable();
    }

    // We're the parent
    guarantee(pid);
    guarantee_err(0 == close(fds[1]), "could not close fd");
    guarantee_err(0 == fcntl(fds[0], F_SETFL, O_NONBLOCK),
                  "could not make socket non-blocking");

    return new worker_t(pid, fds[0]);
}

void supervisor_proc_t::end_worker(workers_t *list, worker_t *worker) {
    // TODO(rntz): Ideally we'd TERM processes, wait a bit, and then KILL them
    // later if they're still alive. But this requires bookkeeping
    // infrastructure that we don't have at present.
    if (kill(worker->pid, SIGKILL)) {
        // Most likely failure reason is if child is already dead, which is
        // weird, but not panic()-worthy.
        fprintf(stderr, "supervisor: failed to kill child %d: %s\n",
                worker->pid, strerror(errno));
    }

    list->remove(worker);
    delete worker;
}

void supervisor_proc_t::panicf(const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    panic();
}

void supervisor_proc_t::panic() {
    end_workers();
    if (kill(engine_pid_, SIGKILL)) {
        fprintf(stderr, "supervisor: Could not kill RethinkDB engine: %s\n",
                strerror(errno));
    }
    exit(EXIT_FAILURE);
}

void supervisor_proc_t::end_workers() {
    worker_t *w;
    while ((w = busy_workers_.head()))
        end_worker(&busy_workers_, w);

    while ((w = idle_workers_.head()))
        end_worker(&idle_workers_, w);
}

void supervisor_proc_t::shut_down() {
    // Clean up.
    guarantee_err(0 == close(engine_fd_), "could not close fd");
    end_workers();

    // Wait for engine process to die.
    int status;
    pid_t res;
    do res = waitpid(engine_pid_, &status, 0);
    while (res == -1 && errno == EINTR);

    guarantee_err(res != -1, "could not waitpid() on RethinkDB engine");
    guarantee(res == engine_pid_);

    // Propagate engine's exit code, if it exited normally.
    exit(WIFEXITED(status) ? WEXITSTATUS(status) : -1);
}

} // namespace jsproc
