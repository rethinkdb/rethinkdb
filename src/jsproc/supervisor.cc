#include "jsproc/supervisor.hpp"

#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>

#include "logger.hpp"

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

        // TODO (rntz): what should we do if close() errors here? if we just
        // die, we'll leave a very confused forked-off rethinkdb engine process
        // lying around.
        close(fds[1]);

        exit(run_supervisor(pid, fds[0]));
    }

    // We're the child. Set up proc->stream to speak to the supervisor.
    guarantee_err(0 == close(fds[1]), "could not close fd");
    proc->stream_.reset(new unix_socket_stream_t(fds[1]));

    return 0;
}

int supervisor_t::run_supervisor(pid_t child, int sockfd) {
    guarantee(0 && "unimplemented");
    (void) child; (void) sockfd;
}

} // namespace jsproc
