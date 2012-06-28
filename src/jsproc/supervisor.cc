#include "jsproc/supervisor.hpp"

#include <cstdio>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <map>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "arch/fd_send_recv.hpp"
#include "jsproc/worker.hpp"
#include "logger.hpp"

namespace jsproc {

supervisor_t::supervisor_t(const info_t &info) :
    stream_(new unix_socket_stream_t(info.socket_fd))
{
    // TODO (rntz): install us as thread-local data on all threads.
}

supervisor_t::~supervisor_t() {}

int supervisor_t::connect(boost::scoped_ptr<unix_socket_stream_t> &stream) {
    // Spawn a socket pair.
    int fds[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    if (res) {
        logERR("Couldn't create socket pair for job request.");
        return res;
    }

    {
        // Send one end down the pipe to the supervisor. To ensure sequencing of
        // write()s, we must be on the supervisor's thread.
        on_thread_t thread_switcher(home_thread());
        res = stream_->send_fd(fds[1]);
    }

    guarantee_err(0 == close(fds[1]), "couldn't close fd");
    if (res) {
        logERR("The supervisor process didn't accept our job request.");
        guarantee_err(0 == close(fds[0]), "couldn't close fd");
        return res;
    }

    // Initialize `stream` with the other end.
    stream.reset(new unix_socket_stream_t(fds[0]));

    return 0;
}

int supervisor_t::spawn(info_t *info) {
    int fds[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    if (res) return res;

    pid_t pid = fork();
    if (pid == -1) {
        guarantee_err(0 == close(fds[0]), "could not close fd");
        guarantee_err(0 == close(fds[1]), "could not close fd");
        return -1;
    }

    if (pid) {
        // We're the parent.
        guarantee_err(0 == close(fds[1]), "could not close fd");
        exec_supervisor(pid, fds[0]);
        unreachable();
    }

    // We're the child. Set up proc->stream to speak to the supervisor.
    guarantee_err(0 == close(fds[0]), "could not close fd");
    info->socket_fd = fds[1];
    return 0;
}


// -------------------- supervisor process --------------------

// TODO (rntz): a panic() method or similar. There are a lot of guarantees etc.
// in here that would be better served by at least attempting to clean up after
// ourselves rather than just abort()ing.

// TODO (rntz): some way of handling engine process shutdown.

// TODO (rntz): a way to kill off unneeded worker processes if we have too many.
// Currently the number of JS procs we have grows monotonically until it hits
// MAX_JS_PROCS.

// Maximum number of open requests for fds that the supervisor will accept.
#define MAX_FD_REQUESTS         1

// The supervisor will attempt to ensure that there are always at least this
// many idle javascript processes.
#define MIN_IDLE_JS_PROCS       1

// The supervisor will attempt to ensure that there are always at least this
// many total javascript processes. This is therefore the size of the initial
// worker pool. Must be at least MIN_IDLE_JS_PROCS.
#define MIN_JS_PROCS            8

// The supervisor will never spawn more than this many JS processes, period.
// This takes priority over MIN_IDLE_JS_PROCS.
#define MAX_JS_PROCS            20

class supervisor_proc_t {
  public:
    supervisor_proc_t(pid_t child, int sockfd);
    ~supervisor_proc_t();

    // Used for spawning new workers. See exec_supervisor(), spawn().
    struct exec_worker_exc_t {
        explicit exec_worker_exc_t(int _fd) : fd(_fd) {}
        int fd;
    };

    struct child_t { pid_t pid; fd_t fd; };

    // Returns exit code of supervisor process (hence of original rethinkdb
    // process).
    int run() THROWS_ONLY(exec_worker_exc_t);

  private:
    // The rethinkdb engine child process.
    child_t engine_;

    // Worker processes.
    typedef std::vector<child_t> workers_t;
    workers_t idle_workers_, busy_workers_;

    // Unfulfilled requests from the engine.
    std::deque<fd_t> requests_;

  private:
    int event_loop() THROWS_ONLY(exec_worker_exc_t);

    void wait_for_events();
    void init_fdsets(int *nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds);
    void get_new_idle_procs();
    void handle_requests();
    void enforce_bounds();

    // Tries to send a request to an idle worker. Returns false on failure. `it`
    // must point to an iterator into idle_workers_ (that is not
    // idle_workers_.end()); it is updated appropriately after return.
    bool send_request(workers_t::iterator *it, fd_t reqfd);

    // Called by signal handler when a child of ours dies.
    void on_sigchild(siginfo_t *info);

    // Returns false on failure.
    bool create_worker(child_t *child) THROWS_ONLY(exec_worker_exc_t);
    void fork_workers(int num);
    void kill_worker(child_t child);

    void self_destruct();

    static void close_workers(workers_t *workers);
};

void supervisor_t::exec_supervisor(pid_t child, int sockfd) THROWS_NOTHING {
    // TODO(rntz): save signal mask/handlers?
    try {
        exit(supervisor_proc_t(child, sockfd).run());
    } catch (supervisor_proc_t::exec_worker_exc_t e) {
        // TODO(rntz): restore signal mask/handlers?
        exec_worker(e.fd);
    }
    unreachable();
}

supervisor_proc_t::supervisor_proc_t(pid_t child, int sockfd) {
    engine_.pid = child;
    engine_.fd = sockfd;
    guarantee(0 == fcntl(engine_.fd, F_SETFL, O_NONBLOCK),
              "could not make socket non-blocking");
}

supervisor_proc_t::~supervisor_proc_t() {
    // Destroy all our workers, to close their fds.
    guarantee_err(0 == close(engine_.fd), "could not close fd");
    close_workers(&idle_workers_);
    close_workers(&busy_workers_);
}

void supervisor_proc_t::close_workers(workers_t *workers) {
    for (workers_t::iterator it = workers->begin(); it != workers->end(); ++it) {
        guarantee_err(0 == close(it->fd), "could not close fd");
    }
}

int supervisor_proc_t::run() THROWS_ONLY(exec_worker_exc_t) {
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
    fork_workers(MIN_JS_PROCS);

    // Main event loop
    // TODO (rntz): determine return code properly!
    // NB. This will be the return code of the original rethinkdb invocation!
    return event_loop();
}

int supervisor_proc_t::event_loop() THROWS_ONLY(exec_worker_exc_t) {
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

    unreachable();              // FIXME
}

void supervisor_proc_t::wait_for_events() {
    // Set up fd set.
    int nfds;
    fd_set readfds, writefds, exceptfds;
    init_fdsets(&nfds, &readfds, &writefds, &exceptfds);

    int res = select(nfds, &readfds, &writefds, &exceptfds, NULL);
    if (res == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
        // Not a problem; just loop round again and we'll handle it.
        res = 0;
    }

    guarantee_err(res != -1, "Waiting for select() failed");

    // Allow signals to get at us.
    sigset_t emptymask, oldmask;
    guarantee_err(0 == sigemptyset(&emptymask), "Could not create empty signal set");
    guarantee_err(0 == sigprocmask(SIG_SETMASK, &emptymask, &oldmask), "Could not unblock signals");
    guarantee_err(0 == sigprocmask(SIG_SETMASK, &oldmask, NULL), "Could not block signals");
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
    for (workers_t::iterator it = busy_workers_.begin(); it != busy_workers_.end(); ++it) {
        addfd(it->fd, readfds, nfds);
    }

    // If we have room on the request queue, we watch the engine fd for read, so
    // that it can send us requests.
    if (requests_.size() < MAX_FD_REQUESTS) {
        addfd(engine_.fd, readfds, nfds);
    }

    // If we have pending requests, we watch all idle processes for write.
    if (!requests_.empty()) {
        for (workers_t::iterator it = idle_workers_.begin(); it != idle_workers_.end(); ++it) {
            addfd(it->fd, writefds, nfds);
        }
    }
}

void supervisor_proc_t::get_new_idle_procs() {
    // For each busy process, check whether we can read from its fd.
    workers_t::iterator it = busy_workers_.begin();
    while (it != busy_workers_.end()) {
        char c;
        ssize_t res = read(it->fd, &c, 1);

        if (res == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Nothing to do; keep going.
            ++it;
            continue;
        }

        if (res == 0 || res == -1) {
            // EOF or shut-down. Worker process is no longer active.
            fprintf(stderr, "supervisor: read from child %d failed (%s), killing it\n",
                    it->pid, strerror(errno));
            kill_worker(*it);
            it = busy_workers_.erase(it);
            continue;
        }

        // Add it to the idle worker list.
        guarantee(res == 1);
        idle_workers_.push_back(*it);
        it = busy_workers_.erase(it);
    }
}

void supervisor_proc_t::handle_requests() {
    workers_t::iterator wi = idle_workers_.begin();

    // Unload requests from the front of the queue.
    std::deque<fd_t>::iterator fdi = requests_.begin();
    while (fdi != requests_.end() && wi != idle_workers_.end()) {
        if (send_request(&wi, *fdi)) {
            // Success! Handle the next request.
            fdi = requests_.erase(fdi);
        }
        // send_request updates `wi` and cleans up for us in case of failure
    }

    // Either we serviced all requests, or we ran out of idle workers which were
    // ready to accept jobs.
    guarantee(requests_.empty() || wi == idle_workers_.end());

    // Read requests from the engine and either send them to an idle worker or
    // put them in the queue, until the queue is full or there are no more
    // requests.
    while (requests_.size() < MAX_FD_REQUESTS) {
        fd_t fd = 0;
        fd_recv_result_t res = recv_fds(engine_.fd, 1, &fd);
        if (res == FD_RECV_ERROR &&
            (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
            // The engine has no more requests, and that's fine by us.
            break;
        }

        switch (res) {
          case FD_RECV_OK:
            // Try to find a willing worker.
            guarantee(fd);
            while (wi != idle_workers_.end()) {
                if (send_request(&wi, fd)) {
                    fd = 0;     // indicates we managed to fulfill the request
                    break;
                }
            }
            if (fd) {
                // Couldn't find a willing worker; push on queue.
                requests_.push_back(fd);
            }
            break;

          case FD_RECV_ERROR:
          case FD_RECV_EOF:
          case FD_RECV_INVALID:
            fprintf(stderr, "supervisor: read from rethinkdb engine failed (%s), shutting down",
                    res == FD_RECV_ERROR ? strerror(errno) :
                    res == FD_RECV_EOF ? "EOF received" :
                    "invalid request - PROBABLY A BUG, PLEASE REPORT");
            self_destruct();

          default: unreachable();
        }
    }
}

bool supervisor_proc_t::send_request(workers_t::iterator *it, fd_t fd) {
    rassert(*it != idle_workers_.end());

    if (!send_fds((*it)->fd, 1, &fd)) {
        // Success! Move worker to busy worker list.
        busy_workers_.push_back(**it);
        *it = idle_workers_.erase(*it);
        return true;
    }

    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        // Worker isn't ready for us. Advance iterator to next candidate.
        ++*it;
    } else {
        // Unknown error. Kill offending worker and update iterator.
        fprintf(stderr, "supervisor: write to child %d failed (%s), killing it\n",
                (*it)->pid, strerror(errno));
        kill_worker(**it);
        *it = idle_workers_.erase(*it);
    }
    return false;
}

void supervisor_proc_t::enforce_bounds() {
    // NB. we count outstanding requests against the number of idle processes we
    // have for checking MIN_IDLE_JS_PROCS.
    int total_procs = idle_workers_.size() + busy_workers_.size();
    int idle_procs = idle_workers_.size() - requests_.size();
    int i = std::min(MAX_JS_PROCS - total_procs,
                     std::max(idle_procs - MIN_IDLE_JS_PROCS,
                              total_procs - MIN_JS_PROCS));

    if (i > 0)
        fork_workers(i);
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
        // TODO (rntz): This should really be a panic, not a guarantee.
        guarantee(0, "Received SIGCHLD for unknown reason");
    }

    pid_t pid = info->si_pid;

    if (pid == engine_.pid) {
        guarantee(0 && "unimplemented");

    } else {
        // Find the worker process that died.
        child_t child;

        for (workers_t::iterator it = busy_workers_.begin(); it != busy_workers_.end(); ++it)
            if (pid == it->pid) {
                child = *it;
                busy_workers_.erase(it);
                goto found;
            }

        for (workers_t::iterator it = idle_workers_.begin(); it != idle_workers_.end(); ++it)
            if (pid == it->pid) {
                child = *it;
                idle_workers_.erase(it);
                goto found;
            }

        // Didn't find the child. TODO (rntz): this should panic, not guarantee.
        guarantee(0, "Got spurious SIGCHLD.");

      found:
        // wait() for it, to clean up zombies.
        printf("Worker %d died with status %d, cleaning up...\n",
               pid, info->si_status);
        pid_t res = waitpid(pid, NULL, WNOHANG);
        guarantee_err(res != -1, "could not waitpid() on dead child");
        guarantee(res != 0, "SIGCHLD reported child dead, but it's alive?");
        guarantee(pid == res);
    }
}

void supervisor_proc_t::fork_workers(int num) {
    guarantee(num >= 0);
    for (int i = 0; i < num; ++i) {
        child_t worker;
        if (!create_worker(&worker))
            continue;
        idle_workers_.push_back(worker);
    }
}

bool supervisor_proc_t::create_worker(child_t *child) THROWS_ONLY(exec_worker_exc_t) {
    int fds[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    if (res) {
        perror("could not make socketpair() for worker process");
        return false;
    }

    pid_t pid = fork();
    if (pid == -1) {
        guarantee_err(0 == close(fds[0]), "could not close fd");
        guarantee_err(0 == close(fds[1]), "could not close fd");
        perror("could not fork() worker process");
        return false;
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

    child->pid = pid;
    child->fd = fds[0];
    return true;
}

void supervisor_proc_t::kill_worker(child_t child) {
    // TODO(rntz): Ideally we'd TERM processes, wait a bit, and then KILL them
    // if they're still alive. But this requires bookkeeping infrastructure that
    // we don't have at present.
    guarantee_err(0 == kill(child.pid, SIGKILL), "could not kill worker process");
    guarantee_err(0 == close(child.fd), "could not close fd");
}

void supervisor_proc_t::self_destruct() {
    guarantee(0 && "unimplemented");
}

} // namespace jsproc
