#ifndef JSPROC_JOB_HPP_
#define JSPROC_JOB_HPP_

#include <stdarg.h>             // va_list

#include "containers/archive/archive.hpp"
#include "containers/archive/fd_stream.hpp"

namespace jsproc {

// Abstract base class for jobs.
class job_t {
  public:
    virtual ~job_t() {}

    // Passed in to a job.
    class control_t : public unix_socket_stream_t {
      private:
        friend void exec_worker(int sockfd);
        control_t(pid_t pid, int fd);

      public:
        void vlog(const char *fmt, va_list ap);
        void log(const char *fmt, ...)
            __attribute__((format (printf, 2, 3)));

      private:
        pid_t pid_;
    };

    typedef void (*func_t)(control_t *);

    // Called on worker process side.
    virtual void run_job(control_t *control) = 0;
};

// Returns 0 on success, -1 on failure.
template <typename instance_t>
int send_job(write_stream_t *stream, const instance_t &job) {
    struct garbage {
        static void job_runner(job_t::control_t *control) {
            // Get the job instance.
            instance_t job;
            archive_result_t res = deserialize(control, &job);
            if (res != ARCHIVE_SUCCESS) {
                control->log("Could not deserialize job: %s",
                             res == ARCHIVE_SOCK_ERROR ? "socket error" :
                             res == ARCHIVE_SOCK_EOF ? "end of file" :
                             res == ARCHIVE_RANGE_ERROR ? "range error" :
                             "unknown error");
                return;
            }

            // Run it.
            job.run_job(control);
        }
    };

    write_message_t msg;

    // This is kind of a hack.
    //
    // We send the address of the function that runs the job we want (namely
    // job_runner). This works only because the address of job_runner is
    // statically known and the worker processes we are sending to are
    // fork()s of ourselves, and so have the same address space layout.
    const job_t::func_t funcptr = &garbage::job_runner;
    msg.append(&funcptr, sizeof funcptr);

    // We send the job over as well; job_runner will deserialize it.
    msg << job;

    return send_write_message(stream, &msg);
}

} // namespace jsproc

#endif // JSPROC_JOB_HPP_
