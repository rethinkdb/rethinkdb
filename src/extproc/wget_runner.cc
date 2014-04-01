// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "extproc/wget_runner.hpp"

#include "extproc/wget_job.hpp"
#include "time.hpp"

wget_runner_t::wget_runner_t(extproc_pool_t *_pool) :
    pool(_pool) { }

wget_result_t wget_runner_t::wget(const std::string &url,
                                  const std::vector<std::string> &headers,
                                  size_t rate_limit,
                                  uint64_t timeout_ms,
                                  signal_t *interruptor) {
    signal_timer_t timeout;
    wait_any_t combined_interruptor(interruptor, &timeout);
    wget_job_t job(pool, &combined_interruptor);

    assert_thread();
    timeout.start(timeout_ms);

    try {
        return job.wget(url, headers, rate_limit);
    } catch (...) {
        // This will mark the worker as errored so we don't try to re-sync with it
        //  on the next line (since we're in a catch statement, we aren't allowed)
        job.worker_error();
        throw;
    }
}

