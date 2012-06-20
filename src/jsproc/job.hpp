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

template <typename instance_t>
class job_t {
  public:
    // Static utility methods.
    static void run_job(job_result_t *result, read_stream_t *input, write_stream_t *output) {
        // Get the job instance.
        instance_t job;
        archive_result_t res = deserialize(input, &job);
        if (res != ARCHIVE_SUCCESS) {
            result->type = JOB_INITIAL_READ_FAILURE;
            result->data.archive_result = res;
            return;
        }

        // Run it.
        job.run(result, input, output);
    }

    static void send_job(write_stream_t *stream, const instance_t &job) {
        write_message_t msg;

        // This is kind of a hack.
        //
        // We send the address of the function that runs the job we want (namely run_job). This
        // works only because the address of run_job is statically known and the worker processes we
        // are sending to are fork()s of ourselves, and so have the same address space layout.
        const job_func_t funcptr = &run_job;
        msg.append(&funcptr, sizeof funcptr);

        // We send the job over as well; run_job will deserialize it.
        msg << job;

        send_write_message(stream, &msg);
    }

    // Instance methods
    virtual void run(job_result_t *result, read_stream_t *input, write_stream_t *output) = 0;
};

// job_source, job_input, and job_output may all alias
void recv_and_run_job(job_result_t *result, read_stream_t *job_source,
                      read_stream_t *job_input, write_stream_t *job_output);

} // namespace jsproc

#endif // JSPROC_JOB_HPP_
