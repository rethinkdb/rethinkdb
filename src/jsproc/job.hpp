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

    // Called on worker process side.
    virtual void run_job(job_result_t *result, read_stream_t *input, write_stream_t *output) = 0;
};

// Returns 0 on success, -1 on failure.
template <typename instance_t>
int send_job(write_stream_t *stream, const instance_t &job) {
    struct garbage {
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
    };

    write_message_t msg;

    // This is kind of a hack.
    //
    // We send the address of the function that runs the job we want (namely
    // job_runner). This works only because the address of job_runner is
    // statically known and the worker processes we are sending to are
    // fork()s of ourselves, and so have the same address space layout.
    const job_func_t funcptr = &garbage::job_runner;
    msg.append(&funcptr, sizeof funcptr);

    // We send the job over as well; job_runner will deserialize it.
    msg << job;

    return send_write_message(stream, &msg);
}

} // namespace jsproc

#endif // JSPROC_JOB_HPP_
