#include "jsproc/spawner.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>             // setpgid

#include "jsproc/job.hpp"
#include "utils.hpp"

namespace jsproc {

spawner_t::spawner_t(const info_t &info) : socket_(info.socket) {}

spawner_t::~spawner_t() {}

pid_t spawner_t::create(info_t *info) {
    int fds[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    if (res) return -1;

    pid_t pid = fork();
    if (-1 == pid) {
        guarantee_err(0 == close(fds[0]), "could not close fd");
        guarantee_err(0 == close(fds[1]), "could not close fd");
        return -1;
    }

    if (0 == pid) {
        // We're the child; run the spawner.
        guarantee_err(0 == close(fds[0]), "could not close fd");
        exec_spawner(fds[1]);
        unreachable();
    }

    // We're the parent. Return.
    guarantee_err(0 == close(fds[1]), "could not close fd");
    info->socket = fds[0];
    return 0;
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

void spawner_t::exec_worker(fd_t sockfd) {
    // TODO (rntz): worker should use prctl() to watch the spawner process.

    // Receive one job and run it.
    job_t::control_t control(getpid(), sockfd);
    job_t::accept_job(&control);
    exit(EXIT_SUCCESS);
}

void spawner_t::exec_spawner(fd_t sockfd) {
    // We set our PGID to our own PID (rather than inheriting our parent's PGID)
    // so that a signal (eg. SIGINT) sent to the parent's PGID (by eg. hitting
    // Ctrl-C at a terminal) will not propagate to us or our children.
    //
    // This is desirable because the RethinkDB engine deliberately crashes with
    // an error message if the spawner or a worker process dies; but a
    // command-line SIGINT should trigger a clean shutdown, not a crash.
    //
    // TODO(rntz): This is an ugly, hackish way to handle the problem.
    guarantee_err(0 == setpgid(0, 0), "spawner could not set PGID");

    // We ignore SIGCHLD so that we don't accumulate zombie children.
    {
        // NB. According to `man 2 sigaction` on linux, POSIX.1-2001 says that
        // this will prevent zombies, but may not be "fully portable".
        struct sigaction act;
        memset(&act, 0, sizeof act);
        act.sa_handler = SIG_IGN;

        guarantee_err(0 == sigaction(SIGCHLD, &act, NULL),
                      "Could not ignore SIGCHLD");
    }

    unix_socket_stream_t sock(sockfd, new blocking_fd_watcher_t());

    for (;;) {
        // Get an fd from our parent.
        fd_t fd;
        archive_result_t res = sock.recv_fd(&fd);
        if (res == ARCHIVE_SOCK_EOF) {
            // Other end shut down cleanly; we should too.
            exit(EXIT_SUCCESS);
        } else if (res != ARCHIVE_SUCCESS) {
            exit(EXIT_FAILURE);
        }

        // Fork a worker process.
        pid_t pid = fork();
        if (-1 == pid)
            exit(EXIT_FAILURE);

        if (0 == pid) {
            // We're the child.
            exec_worker(fd);
            unreachable();
        }

        // We're the parent.
        guarantee_err(0 == close(fd), "couldn't close fd");

        // Send back its pid.
        guarantee(sizeof pid == sock.write(&pid, sizeof pid));
    }
    unreachable();
}

} // namespace jsproc
