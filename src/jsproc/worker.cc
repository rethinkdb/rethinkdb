#include "jsproc/worker.hpp"

#include <cstring>              // strerror
#include <sys/types.h>
#include <sys/socket.h>

namespace jsproc {

// -------------------- worker_t --------------------
int worker_t::spawn(worker_t *proc) {
    int fds[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    if (res) return res;

    pid_t pid = fork();
    if (pid == -1) {
        guarantee_err(0 == close(fds[0]), "could not close fd");
        guarantee_err(0 == close(fds[1]), "could not close fd");
        return -1;
    }

    if (!pid) {
        // We're the child.
        guarantee_err(0 == close(fds[0]), "could not close fd");
        exit(worker_t::run_worker(fds[1]));
    }

    // We're the parent
    guarantee(pid);
    guarantee_err(0 == close(fds[1]), "could not close fd");

    proc->pid = pid;
    proc->fd = fds[0];
    return 0;
}

// Accepts and runs a job.
static void accept_job(
    job_result_t *result,
    read_stream_t *job_source,
    read_stream_t *job_input,
    write_stream_t *job_output)
{
    // Try to receive the job.
    job_func_t jobfunc;
    int64_t res = force_read(job_source, &jobfunc, sizeof jobfunc);
    if (res < (int64_t) sizeof jobfunc) {
        result->type = JOB_INITIAL_READ_FAILURE;
        result->data.archive_result = res == -1 ? ARCHIVE_SOCK_ERROR : ARCHIVE_SOCK_EOF;
        return;
    }

    // The job should explicitly set result->type. Otherwise, something weird
    // happened.
    result->type = JOB_UNKNOWN_ERROR;
    (*jobfunc)(result, job_input, job_output);
}

int worker_t::run_worker(int sockfd) {
    unix_socket_stream_t sock(sockfd, new blocking_fd_watcher_t());

    // Receive and run jobs until we get an error, or a job tells us to shut down.
    job_result_t result;
    do {
        accept_job(&result, &sock, &sock, &sock);
        // TODO (rntz): logging, error-detection etc.
    } while (result.type != JOB_SHUTDOWN && sock.is_read_open() && sock.is_write_open());

    // TODO (rntz): useful return codes.
    return 0;
}

} // namespace jsproc
