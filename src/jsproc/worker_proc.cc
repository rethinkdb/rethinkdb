#include "jsproc/worker_proc.hpp"

#include <cstdio>               // fprintf, stderr
#include <cstring>              // strerror
#include <sys/socket.h>
#include <sys/types.h>

#include "containers/archive/fd_stream.hpp"
#include "jsproc/job.hpp"
#include "utils.hpp"

namespace jsproc {

job_t::control_t::control_t(int pid, int fd) :
    pid_(pid), socket_(fd, new blocking_fd_watcher_t()) {}

job_t::control_t::~control_t() {}

void job_t::control_t::vlog(const char *fmt, va_list ap) {
    std::string real_fmt = strprintf("[%d] worker: %s\n", pid_, fmt);
    vfprintf(stderr, real_fmt.c_str(), ap);
}

void job_t::control_t::log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(fmt, ap);
    va_end(ap);
}

// Accepts and runs a job.
static void handle_job(job_t::control_t *control) {
    // Try to receive the job.
    job_t::func_t jobfunc;
    int64_t res = force_read(control, &jobfunc, sizeof jobfunc);
    if (res < (int64_t) sizeof jobfunc) {
        control->log("Couldn't read job function: %s",
                      res == -1 ? strerror(errno) : "end-of-file received");
        return;
    }

    // Run the job.
    (*jobfunc)(control);

    // Wait for the other side to shut down. This ensures that it has gotten all
    // of the data we sent. TODO(rntz): figure out if there's a better way to
    // make sure our data gets delivered.
    char c;
    res = control->read(&c, 1);

    // TODO(rntz): receiving data on the connection after the job function
    // finishes indicates a bug in the code on the other end (ie. in the
    // rethinkdb engine), and this should ideally be propagated somehow.
    guarantee(res != 1, "received extra data after job finished");
    guarantee(res == 0, "other end of job didn't shut down properly");
}

void exec_worker(int sockfd) {
    pid_t mypid = getpid();
    unix_socket_stream_t sock(sockfd, new blocking_fd_watcher_t());

    // Receive and run jobs until we get an error, or a job tells us to shut down.
    for (;;) {
        int fd;
        archive_result_t res = sock.recv_fd(&fd);
        if (res != ARCHIVE_SUCCESS) {
            fprintf(stderr, "[%d] worker: unexpected %s trying to receive job\n",
                    mypid,
                    res == ARCHIVE_SOCK_ERROR ? "socket error" :
                    res == ARCHIVE_SOCK_EOF ? "EOF" :
                    res == ARCHIVE_RANGE_ERROR ? "range error" :
                    "error");
            break;
        }

        {
            job_t::control_t control(mypid, fd);
            handle_job(&control);
        }

        // Let the supervisor know that we're ready again.
        char c = '\0';
        guarantee(1 == sock.write(&c, 1));
    }

    // The way we are "supposed to" die is to be killed by the supervisor. So we
    // always exit with failure.
    exit(EXIT_FAILURE);
}

} // namespace jsproc
