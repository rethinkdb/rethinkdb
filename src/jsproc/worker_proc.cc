#include "jsproc/worker_proc.hpp"

#include <cstring>              // strerror
#include <sys/types.h>
#include <sys/socket.h>

#include "containers/archive/fd_stream.hpp"
#include "jsproc/job.hpp"

namespace jsproc {

// Accepts and runs a job.
static void handle_job(job_result_t *result, int fd) {
    unix_socket_stream_t stream(fd, new blocking_fd_watcher_t());

    // Try to receive the job.
    job_func_t jobfunc;
    int64_t res = force_read(&stream, &jobfunc, sizeof jobfunc);
    if (res < (int64_t) sizeof jobfunc) {
        result->type = JOB_INITIAL_READ_FAILURE;
        result->data.archive_result = res == -1 ? ARCHIVE_SOCK_ERROR : ARCHIVE_SOCK_EOF;
        return;
    }

    // The job should explicitly set result->type. Otherwise, something weird
    // happened.
    result->type = JOB_UNKNOWN_ERROR;
    (*jobfunc)(result, &stream, &stream);

    if (result->type == JOB_SUCCESS) {
        // Wait for the other side to shut down. This ensures that it has gotten
        // all of the data we sent. TODO(rntz): figure out if there's a better
        // way to make sure our data gets delivered.
        char c;
        res = stream.read(&c, 1);

        // TODO(rntz): receiving data on the connection after the job function
        // finishes indicates a bug in the code on the other end (ie. in the
        // rethinkdb engine), and this should ideally be propagated somehow.
        guarantee(res != 1, "received extra data after job finished");
        guarantee(res == 0, "other end of job didn't shut down properly");
    }
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

        job_result_t result;
        handle_job(&result, fd);

        if (result.type != JOB_SUCCESS) {
            fprintf(stderr, "[%d] worker: failed to run job: %s\n",
                    mypid,
                    result.type == JOB_INITIAL_READ_FAILURE ? "couldn't read job" :
                    result.type == JOB_READ_FAILURE ? "couldn't read data during job" :
                    result.type == JOB_WRITE_FAILURE ? "couldn't write data during job" :
                    "unknown error");
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
