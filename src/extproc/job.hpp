// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef EXTPROC_JOB_HPP_
#define EXTPROC_JOB_HPP_

#include <stdarg.h>             // va_list

#include "arch/runtime/runtime_utils.hpp" // fd_t
#include "containers/archive/archive.hpp"
#include "containers/archive/socket_stream.hpp"

namespace extproc {

// Abstract base class for jobs.
class job_t {
  public:
    virtual ~job_t() {}

    // Passed in to a job on the worker process side.
    class control_t : public unix_socket_stream_t {
      private:
        friend class spawner_t;
        control_t(pid_t pid, scoped_fd_t *fd);

      public:
        void vlog(const char *fmt, va_list ap);
        void log(const char *fmt, ...)
            __attribute__((format (printf, 2, 3)));

      private:
        pid_t pid_;
    };

    // Sends us over a stream. The recipient must be a fork()ed child (or
    // grandchild, or parent, etc) of us. Returns 0 on success, -1 on error.
    MUST_USE int send_over(write_stream_t *stream) const;

    // Appends us to a write message. Same constraints on recipient as above.
    void append_to(write_message_t *message) const;

    // Receives and runs a job. Called on worker process side. `extra` is passed
    // to the accepted job's run_job() method.
    //
    // Returns 0 on success, -1 on failure.
    static int accept_job(control_t *control, void *extra);

    /* ----- Pure virtual methods ----- */

    // Called on worker process side. `extra` comes from accept_job(); it's a
    // way for job acceptors to pass data to the jobs they accept. (It's quite
    // normal for it to be unused/ignored.)
    virtual void run_job(control_t *control, void *extra) = 0;

    // Returns a function that deserializes & runs an instance of the
    // appropriate job type. Called on worker process side.
    typedef void (*func_t)(control_t*, void*);
    virtual func_t job_runner() const = 0;

    // Serialization methods. Suggest implementing by invoking
    // RDB_MAKE_ME_SERIALIZABLE_#(..) in subclass definition.
    friend class write_message_t;
    friend class archive_deserializer_t;
    virtual void rdb_serialize(write_message_t &msg /* NOLINT */) const = 0;
    virtual archive_result_t rdb_deserialize(read_stream_t *s) = 0;
};

// NB. base_job_t had better descend from job_t.
template <class instance_t, class base_job_t = job_t>
class auto_job_t : public base_job_t {
    static void job_runner_func(job_t::control_t *control, void *extra) {
        // Get the job instance.
        instance_t job;
        archive_result_t res = deserialize(control, &job);

        if (res != ARCHIVE_SUCCESS) {
            control->log("Could not deserialize job: %s",
                         res == ARCHIVE_SOCK_ERROR ? "socket error" :
                         res == ARCHIVE_SOCK_EOF ? "end of file" :
                         res == ARCHIVE_RANGE_ERROR ? "range error" :
                         "unknown error");
            crash("worker: could not deserialize job");
        }

        // Run it.
        job.run_job(control, extra);
    }

    virtual job_t::func_t job_runner() const { return job_runner_func; }
};

} // namespace extproc

#endif // EXTPROC_JOB_HPP_
