// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "extproc/spawner.hpp"

#include <signal.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "arch/fd_send_recv.hpp"
#include "extproc/job.hpp"
#include "utils.hpp"

namespace extproc {

void exec_spawner(fd_t socket) NORETURN;
void exec_worker(pid_t spawner_pid, fd_t socket) NORETURN;



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
    const pid_t pid = ::waitpid(-1, &status, WNOHANG);
    guarantee_err(-1 != pid, "could not waitpid()");

    // We might spawn processes other than the spawner, whose deaths we ignore.
    // waitpid() might also return 0, indicating some event other than child
    // process termination (ie. SIGSTOP or SIGCONT). We ignore this.
    if (pid == spawner_pid) {
        crash_or_trap("Spawner process died!");
    }
}

spawner_t::spawner_t(spawner_info_t *info)
    : pid_(info->pid), socket_(&info->socket)
{
    guarantee(-1 == spawner_pid);
    spawner_pid = pid_;

    // Check that the spawner hasn't already exited.
    const pid_t pid = ::waitpid(pid_, NULL, WNOHANG);
    guarantee(0 == pid, "spawner process already died!");

    // Establish SIGCHLD handler for spawner.
    struct sigaction sa = make_sa_handler(0, sigchld_handler);
    const int res = sigaction(SIGCHLD, &sa, NULL);
    guarantee_err(res == 0, "could not set SIGCHLD handler");
}

spawner_t::~spawner_t() {
    // De-establish SIGCHLD handler.
    struct sigaction sa = make_sa_handler(0, SIG_DFL);
    const int res = sigaction(SIGCHLD, &sa, NULL);
    guarantee_err(res == 0, "could not unset SIGCHLD handler");

    guarantee(spawner_pid > 0);
    spawner_pid = -1;
}

void spawner_t::create(spawner_info_t *info) {
    int fds[2];
    const int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    guarantee_err(0 == res, "could not create socketpair for spawner");

    const pid_t pid = fork();
    guarantee_err(-1 != pid, "could not fork spawner process");

    if (0 == pid) {
        // We're the child; run the spawner.
        const int closeres = ::close(fds[0]);
        guarantee_err(0 == closeres || errno == EINTR, "could not close fd");
        exec_spawner(fds[1]);
        unreachable();
    }

    // We're the parent. Return.
    const int closeres = ::close(fds[1]);
    guarantee_err(0 == closeres || errno == EINTR, "could not close fd");
    info->pid = pid;
    info->socket.reset(fds[0]);
}

pid_t spawner_t::spawn_process(scoped_fd_t *socket) {
    assert_thread();

    // Create a socket pair.
    fd_t fds[2];
    const int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    if (res != 0) {
        return -1;
    }

    // We need to send the fd & receive the pid atomically with respect to other
    // calls to spawn_process().
    mutex_t::acq_t lock(&mutex_);

    // Send one half to the spawner process.
    const int send_fd_res = socket_.send_fd(fds[1]);
    guarantee(0 == send_fd_res);
    const int closeres = ::close(fds[1]);
    guarantee_err(0 == closeres || errno == EINTR, "could not close fd");
    socket->reset(fds[0]);

    // Receive the pid from the spawner process.
    pid_t pid;
    const int64_t read_res = force_read(&socket_, &pid, sizeof(pid));
    guarantee(sizeof(pid) == read_res);
    guarantee(pid > 0);

    return pid;
}


// ---------- Spawner & worker processes ----------
// Runs the spawner process. Does not return.
void exec_spawner(fd_t socket) {
    // We set our PGID to our own PID (rather than inheriting our parent's PGID)
    // so that a signal (eg. SIGINT) sent to the parent's PGID (by eg. hitting
    // Ctrl-C at a terminal) will not propagate to us or our children.
    //
    // This is desirable because the RethinkDB engine deliberately crashes with
    // an error message if the spawner or a worker process dies; but a
    // command-line SIGINT should trigger a clean shutdown, not a crash.
    //
    // TODO(rntz): This is an ugly, hackish way to handle the problem.
    const int setpgid_res = setpgid(0, 0);
    guarantee_err(0 == setpgid_res, "spawner: could not set PGID");

    // We ignore SIGCHLD so that we don't accumulate zombie children.
    {
        // NB. According to `man 2 sigaction` on linux, POSIX.1-2001 says that
        // this will prevent zombies, but may not be "fully portable".
        struct sigaction sa = make_sa_handler(0, SIG_IGN);
        const int res = sigaction(SIGCHLD, &sa, NULL);
        guarantee_err(res == 0, "spawner: Could not ignore SIGCHLD");
    }

    pid_t local_spawner_pid = getpid();

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
            const int closeres = ::close(socket);
            guarantee_err(0 == closeres || errno == EINTR, "worker: could not close fd");
            exec_worker(local_spawner_pid, fd);
            unreachable();
        }

        // We're the parent.
        const int closeres = ::close(fd);
        guarantee_err(0 == closeres || errno == EINTR, "spawner: couldn't close fd");

        // Send back its pid. Wish we could use unix_socket_stream_t for this,
        // but its destructor calls shutdown(), and we can't have that happening
        // in the worker child.
        const char *buf = reinterpret_cast<const char *>(&pid);
        size_t sz = sizeof(pid);
        while (sz) {
            ssize_t res;
            do {
                res = ::write(socket, buf, sz);
            } while (res == -1 && errno == EINTR);

            guarantee_err(res > 0, "spawner: could not write() to engine socket");
            guarantee(static_cast<size_t>(res) <= sz);
            sz -= res;
            buf += res;
        }
    }
}

// On OS X, we have to poll the ppid to detect when the parent process dies.  So we use SIGALRM and
// setitimer to do that.  (We could also use kqueue to do this without polling, in a separate
// thread.  We could also just make a separate thread and poll from it.)  We have to make this
// global variable because setitimer doesn't let you pass the value through a siginfo_t parameter.
pid_t spawner_pid_for_sigalrm = 0;

void check_ppid_for_death(int) {
    pid_t ppid = getppid();
    if (spawner_pid_for_sigalrm != 0 && spawner_pid_for_sigalrm != ppid) {
        // We didn't fail, so should we exit with failure?  This status code doesn't really matter.
        _exit(EXIT_FAILURE);
    }
}

// Runs the worker process. Does not return.
void exec_worker(pid_t local_spawner_pid, fd_t sockfd) {
    // Make sure we die when our parent dies.  (The parent, the spawner process, dies when the
    // rethinkdb process dies.)
    {
        spawner_pid_for_sigalrm = local_spawner_pid;

        struct sigaction sa = make_sa_handler(0, check_ppid_for_death);
        const int sigaction_res = sigaction(SIGALRM, &sa, NULL);
        guarantee_err(sigaction_res == 0, "worker: could not set action for ALRM signal");

        struct itimerval timerval;
        timerval.it_interval.tv_sec = 0;
        timerval.it_interval.tv_usec = 500 * THOUSAND;
        timerval.it_value = timerval.it_interval;
        struct itimerval old_timerval;
        const int itimer_res = setitimer(ITIMER_REAL, &timerval, &old_timerval);
        guarantee_err(itimer_res == 0, "worker: setitimer failed");
        guarantee(old_timerval.it_value.tv_sec == 0 && old_timerval.it_value.tv_usec == 0,
                  "worker: setitimer saw that we already had an itimer!");
    }

    // Receive one job and run it.
    scoped_fd_t fd(sockfd);
    job_control_t control(getpid(), local_spawner_pid, &fd);
    exit(job_t::accept_job(&control, NULL));
}

} // namespace extproc
