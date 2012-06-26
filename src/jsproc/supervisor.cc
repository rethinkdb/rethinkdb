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
#include <unistd.h>

#include "logger.hpp"
#include "jsproc/worker.hpp"

namespace jsproc {

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

int supervisor_t::spawn(supervisor_t *proc) {
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
    guarantee_err(0 == close(fds[1]), "could not close fd");
    proc->stream_.reset(new unix_socket_stream_t(fds[1]));

    return 0;
}


// -------------------- supervisor process --------------------

// TODO (rntz): some way of handling engine process shutdown.

// Maximum number of open requests for fds that the supervisor will accept.
#define MAX_FD_REQUESTS         1

// The supervisor will attempt to ensure that there are always at least this
// many idle javascript processes.
#define MIN_IDLE_JS_PROCS       1

// The supervisor will attempt to ensure that there are always at least this
// many total javascript processes. This is therefore the size of the initial
// worker pool. Must be at least MIN_IDLE_JS_PROCS.
#define MIN_JS_PROCS            8

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

    // If this returns, it means we are a forked worker process. The returned
    // int is the fd of our control socket.
    int exec() THROWS_ONLY(exec_worker_exc_t);

    // Returns exit code of supervisor process (hence of original rethinkdb
    // process).
    int run() THROWS_ONLY(exec_worker_exc_t);
    int event_loop() THROWS_ONLY(exec_worker_exc_t);

    // Various helpers for event_loop().
    void wait_for_events();
    void init_fdsets(int *nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds);
    void get_new_idle_procs();
    void handle_requests();
    void enforce_bounds();

    // Called by signal handler when a child of ours dies.
    void on_child_death(siginfo_t *info);

    // Returns false on failure.
    bool create_worker(child_t *child) THROWS_ONLY(exec_worker_exc_t);
    void fork_workers(int num);

  private:
    // The rethinkdb engine child process.
    child_t engine_;

    // Worker processes.
    typedef std::vector<child_t> workers_t;
    workers_t idle_workers_, busy_workers_;

    // Unfulfilled requests from the engine.
    std::deque<fd_t> engine_requests_;

    static void close_workers(workers_t *workers);
};

void supervisor_t::exec_supervisor(pid_t child, int sockfd) THROWS_NOTHING {
    // TODO(rntz): save signal handlers?
    try {
        supervisor_proc_t(child, sockfd).run();
    } catch (supervisor_proc_t::exec_worker_exc_t e) {
        // TODO(rntz): restore signal handlers?
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

int supervisor_proc_t::exec() THROWS_ONLY(exec_worker_exc_t) {
    exit(run());
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
            supervisor_proc->on_child_death(info);
        }

        sa_setter_t() {
            struct sigaction sa;
            guarantee(0 == sigemptyset(&sa.sa_mask), "Could not create empty signal set");
            sa.sa_flags = SA_SIGINFO;
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

    // TODO (rntz) Main event loop

    // TODO (rntz): determine return code properly!
    // NB. This will be the return code of the original rethinkdb invocation!
    return -1;
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

void supervisor_proc_t::init_fdsets(int *nfds, fd_set *readfds, fd_set *writefds, fd_set *except_fds) {
    guarantee(0 && "unimplemented");
    (void) nfds; (void) readfds; (void) writefds; (void) except_fds;
}

void supervisor_proc_t::get_new_idle_procs() {
    guarantee(0 && "unimplemented");
}

void supervisor_proc_t::handle_requests() {
    guarantee(0 && "unimplemented");
}

void supervisor_proc_t::enforce_bounds() {
    guarantee(0 && "unimplemented");
}

// reason is the value of si_code
void supervisor_proc_t::on_child_death(siginfo_t *info) {
    guarantee(0 && "unimplemented");
    (void) info;
}

void supervisor_proc_t::fork_workers(int num) {
    guarantee(num > 0);
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

} // namespace jsproc
