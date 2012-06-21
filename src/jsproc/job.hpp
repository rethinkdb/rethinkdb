#ifndef JSPROC_JOB_HPP_
#define JSPROC_JOB_HPP_

#include "containers/archive/archive.hpp"

namespace jsproc {

// Declared out here so that its branches are visible at top-level. Only
// actually used in job_result_t.
enum job_result_type_t {
    JOB_SUCCESS = 0,
    JOB_UNKNOWN_ERROR,

    // If we couldn't deserialize the initial job function or instance.
    // `data.archive_result` contains more info.
    JOB_INITIAL_READ_FAILURE,

    // If we couldn't deserialize something during the job's execution itself.
    // `data.archive_result` contains more info.
    JOB_READ_FAILURE,

    // If writing failed at some point during the job's execution. No additional
    // data.
    JOB_WRITE_FAILURE,

    // Indicates that the worker process executing this job should shut down.
    JOB_SHUTDOWN,
};

struct job_result_t {
    job_result_type_t type;
    // Extra data regarding the failure mode.
    union {
        archive_result_t archive_result;
    } data;
};

typedef void (*job_func_t)(job_result_t *result, read_stream_t *read, write_stream_t *write);

// Abstract base class for jobs.
class job_t {
  public:
    virtual ~job_t() {}

    // Called on main process side.
    virtual void send(write_stream_t *stream) = 0;
    virtual void rdb_serialize(write_message_t &msg) = 0;

    // Called on worker process side.
    virtual archive_result_t rdb_deserialize(read_stream_t *s) = 0;
    virtual void run_job(job_result_t *result, read_stream_t *input, write_stream_t *output) = 0;
};

template <typename instance_t>
class magic_job_t : public job_t {
  public:
    // The job_func_t that runs this kind of job.
    static void job_runner(job_result_t *result, read_stream_t *input, write_stream_t *output) {
        // Get the job instance.
        instance_t job;
        archive_result_t res = deserialize(input, &job);
        if (res != ARCHIVE_SUCCESS) {
            result->type = JOB_INITIAL_READ_FAILURE;
            result->data.archive_result = res;
            return;
        }

        // Run it.
        job.run_job(result, input, output);
    }

    void send(write_stream_t *stream) {
        write_message_t msg;

        // This is kind of a hack.
        //
        // We send the address of the function that runs the job we want (namely
        // job_runner). This works only because the address of job_runner is
        // statically known and the worker processes we are sending to are
        // fork()s of ourselves, and so have the same address space layout.
        const job_func_t funcptr = &job_runner;
        msg.append(&funcptr, sizeof funcptr);

        // We send ourselves over as well; run_job will deserialize us.
        rdb_serialize(msg);

        send_write_message(stream, &msg);
    }
};

} // namespace jsproc

#endif // JSPROC_JOB_HPP_
