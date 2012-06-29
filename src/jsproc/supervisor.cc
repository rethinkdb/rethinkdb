#include "jsproc/supervisor.hpp"

#include <sys/types.h>          // socketpair()
#include <sys/socket.h>         // socketpair()
#include <sys/un.h>             // AF_UNIX

#include "jsproc/supervisor_proc.hpp"
#include "jsproc/worker_proc.hpp"
#include "logger.hpp"

namespace jsproc {

supervisor_t::supervisor_t(const info_t &info) :
    stream_(new unix_socket_stream_t(info.socket_fd)) {}

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

void supervisor_t::exec_supervisor(pid_t child, int sockfd) THROWS_NOTHING {
    try {
        supervisor_proc_t(child, sockfd).run();
    } catch (supervisor_proc_t::exec_worker_exc_t e) {
        exec_worker(e.fd);
    }
    unreachable();
}

} // namespace jsproc
