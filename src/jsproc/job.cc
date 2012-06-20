#include "jsproc/job.hpp"

namespace jsproc {

void recv_and_run_job(job_result_t *result,
                      // job_source, job_input, and job_output may all alias
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

} // namespace jsproc
