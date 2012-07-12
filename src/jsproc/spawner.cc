#include "jsproc/spawner.hpp"

#include <signal.h>             // sigaction
#include <sys/prctl.h>          // prctl
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>           // waitpid
#include <unistd.h>             // setpgid

#include "arch/fd_send_recv.hpp"
#include "jsproc/job.hpp"
#include "utils.hpp"

namespace jsproc {

// Checks that we only create one spawner. This is an ugly restriction, but it
// means we can put the SIGCHLD-handling logic in here, so that it is properly
// scoped to the lifetime of the spawner.
//
// TODO(rntz): More general way of handling SIGCHLD.
static pid_t spawner_pid = -1;

static void sigchld_handler(int signo) {
    guarantee(signo == SIGCHLD);
    guarantee(spawner_pid > 0);

    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    guarantee_err(-1 != pid, "could not waitpid()");

    // We might spawn processes other than the spawner, whose deaths we ignore.
    // waitpid() might also return 0, indicating some event other than child
    // process termination (ie. SIGSTOP or SIGCONT). We ignore this.
    if (pid == spawner_pid) {
        crash_or_trap("Spawner process died!");
    }
}

spawner_t::spawner_t(const info_t &info)
    : pid_(info.pid), socket_(info.socket)
{
    guarantee(-1 == spawner_pid);
    spawner_pid = pid_;

    // Check that the spawner hasn't already exited.
    guarantee(0 == waitpid(pid_, NULL, WNOHANG),
              "spawner process already died!");

    // Establish SIGCHLD handler for spawner.
    struct sigaction act;
    memset(&act, 0, sizeof act);
    act.sa_handler = sigchld_handler;
    guarantee_err(0 == sigemptyset(&act.sa_mask), "could not empty signal mask");
    guarantee_err(0 == sigaction(SIGCHLD, &act, NULL), "could not set SIGCHLD handler");
}

spawner_t::~spawner_t() {
    // De-establish SIGCHLD handler.
    struct sigaction act;
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_DFL;
    guarantee_err(0 == sigemptyset(&act.sa_mask), "could not empty signal mask");
    guarantee_err(0 == sigaction(SIGCHLD, &act, NULL), "could not unset SIGCHLD handler");

    guarantee(spawner_pid > 0);
    spawner_pid = -1;
}

void spawner_t::create(info_t *info) {
    int fds[2];
    guarantee_err(0 == socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
                  "could not create socketpair for spawner");

    pid_t pid = fork();
    guarantee_err(-1 != pid, "could not fork spawner process");

    if (0 == pid) {
        // We're the child; run the spawner.
        guarantee_err(0 == close(fds[0]), "could not close fd");
        exec_spawner(fds[1]);
        unreachable();
    }

    // We're the parent. Return.
    guarantee_err(0 == close(fds[1]), "could not close fd");
    info->pid = pid;
    info->socket = fds[0];
}

pid_t spawner_t::spawn_process(fd_t *socket) {
    assert_thread();

    // Create a socket pair.
    fd_t fds[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    if (res) return -1;

    // We need to send the fd & receive the pid atomically with respect to other
    // calls to spawn_process().
    mutex_t::acq_t lock(&mutex_);

    // Send one half to the spawner process.
    guarantee(0 == socket_.send_fd(fds[1]));
    guarantee_err(0 == close(fds[1]), "could not close fd");
    *socket = fds[0];

    // Receive the pid from the spawner process.
    pid_t pid;
    guarantee(sizeof pid == force_read(&socket_, &pid, sizeof pid));
    guarantee(pid > 0);

    return pid;
}


// ---------- Spawner & worker processes ----------
// Runs the spawner process. Does not return.
void spawner_t::exec_spawner(fd_t socket) {
    // We set our PGID to our own PID (rather than inheriting our parent's PGID)
    // so that a signal (eg. SIGINT) sent to the parent's PGID (by eg. hitting
    // Ctrl-C at a terminal) will not propagate to us or our children.
    //
    // This is desirable because the RethinkDB engine deliberately crashes with
    // an error message if the spawner or a worker process dies; but a
    // command-line SIGINT should trigger a clean shutdown, not a crash.
    //
    // TODO(rntz): This is an ugly, hackish way to handle the problem.
    guarantee_err(0 == setpgid(0, 0), "spawner: could not set PGID");

    // We ignore SIGCHLD so that we don't accumulate zombie children.
    {
        // NB. According to `man 2 sigaction` on linux, POSIX.1-2001 says that
        // this will prevent zombies, but may not be "fully portable".
        struct sigaction act;
        memset(&act, 0, sizeof act);
        act.sa_handler = SIG_IGN;

        guarantee_err(0 == sigaction(SIGCHLD, &act, NULL),
                      "spawner: Could not ignore SIGCHLD");
    }

    for (;;) {
        // Get an fd from our parent.
        fd_t fd;
        fd_recv_result_t fdres = recv_fds(socket, 1, &fd);
        if (fdres == FD_RECV_EOF) {
            // Other end shut down cleanly; we should too.
            exit(EXIT_SUCCESS);
        } else if (fdres != FD_RECV_OK) {
            exit(EXIT_FAILURE);
        }

        // Fork a worker process.
        pid_t pid = fork();
        if (-1 == pid)
            exit(EXIT_FAILURE);

        if (0 == pid) {
            // We're the child/worker.
            guarantee_err(0 == close(socket), "worker: could not close fd");
            exec_worker(fd);
            unreachable();
        }

        // We're the parent.
        guarantee_err(0 == close(fd), "spawner: couldn't close fd");

        // Send back its pid. Wish we could use unix_socket_stream_t for this,
        // but its destructor calls shutdown(), and we can't have that happening
        // in the worker child.
        const char *buf = reinterpret_cast<const char *>(&pid);
        size_t sz = sizeof pid;
        while (sz) {
            ssize_t w = write(socket, buf, sz);
            if (w == -1 && errno == EINTR) continue;
            guarantee_err(w > 0, "spawner: could not write() to engine socket");
            guarantee((size_t)w <= sz);
            sz -= w, buf += w;
        }
    }
}

// Runs the worker process. Does not return.
void spawner_t::exec_worker(fd_t sockfd) {
    // Makes sure we get SIGTERMed when our parent (the spawner) dies.
    // TODO(rntz): prctl is linux-specific.
    guarantee_err(0 == prctl(PR_SET_PDEATHSIG, SIGTERM),
                  "worker: could not set parent-death signal");

    // Receive one job and run it.
    job_t::control_t control(getpid(), sockfd);
    exit(job_t::accept_job(&control));
}

} // namespace jsproc
