#include "jsproc/supervisor.hpp"

#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>

#include "arch/runtime/event_queue.hpp"
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

        // TODO (rntz): what should we do if close() errors here? if we just
        // die, we'll leave a very confused forked-off rethinkdb engine process
        // lying around.
        close(fds[1]);

        exec_supervisor(pid, fds[0]);
        unreachable();
    }

    // We're the child. Set up proc->stream to speak to the supervisor.
    guarantee_err(0 == close(fds[1]), "could not close fd");
    proc->stream_.reset(new unix_socket_stream_t(fds[1]));

    return 0;
}


// -------------------- supervisor process --------------------
class supervisor_proc_t :
        private linux_queue_parent_t, private linux_event_callback_t
{
    // The PID and unix socket for the rethinkdb engine child process.
    pid_t engine_pid_;
    fd_t engine_fd_;

    // Queue used to multiplex events.
    linux_event_queue_t queue_;

  public:
    supervisor_proc_t(pid_t child, int sockfd);

    // Used for spawning new workers. See exec_supervisor(), spawn().
    struct exec_worker_exc_t {
        explicit exec_worker_exc_t(int _fd) : fd(_fd) {}
        int fd;
    };

    struct worker_t { pid_t pid; fd_t fd; };

    // If this returns, it means we are a forked worker process. The returned
    // int is the fd of our control socket.
    int run() THROWS_ONLY(exec_worker_exc_t);

    // linux_queue_parent_t methods
    virtual void pump();
    virtual bool should_shut_down();

    // linux_event_callback_t methods
    virtual void on_event(int events);

    // Returns true on success, false on failure.
    bool spawn(worker_t *worker) THROWS_ONLY(exec_worker_exc_t);
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

supervisor_proc_t::supervisor_proc_t(pid_t child, int sockfd) :
    engine_pid_(child), engine_fd_(sockfd),
    queue_(this)
{}

int supervisor_proc_t::run() THROWS_ONLY(exec_worker_exc_t) {
    // TODO (rntz): set up signal handlers.

    // TODO (rntz): set up event handlers on queue_
    queue_.watch_resource(engine_fd_, poll_event_in, this);

    // Run event loop.
    queue_.run();

    // TODO (rntz): determine return code properly!
    // NB. This will be the return code of the original rethinkdb invocation!
    exit(-1);
}

bool supervisor_proc_t::spawn(worker_t *proc) THROWS_ONLY(exec_worker_exc_t) {
    int fds[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    if (res) {
        // FIXME: error message.
        return false;
    }

    pid_t pid = fork();
    if (pid == -1) {
        guarantee_err(0 == close(fds[0]), "could not close fd");
        guarantee_err(0 == close(fds[1]), "could not close fd");
        // FIXME: error message
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

    proc->pid = pid;
    proc->fd = fds[0];
    return true;
}

void supervisor_proc_t::pump() {
    guarantee(0 && "unimplemented");
}

bool supervisor_proc_t::should_shut_down() {
    guarantee(0 && "unimplemented");
    return false;
}

// Called when engine_fd_ has events for us.
void supervisor_proc_t::on_event(int events) {
    // FIXME
    guarantee(0 && "unimplemented");
    (void) events;
}

} // namespace jsproc
