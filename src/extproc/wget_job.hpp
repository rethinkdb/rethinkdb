// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef EXTPROC_WGET_JOB_HPP_
#define EXTPROC_WGET_JOB_HPP_

#include <vector>
#include <string>

#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "utils.hpp"
#include "containers/archive/archive.hpp"
#include "containers/counted.hpp"
#include "concurrency/signal.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_job.hpp"
#include "extproc/wget_runner.hpp"
#include "rdb_protocol/datum.hpp"

class wget_job_t {
public:
    wget_job_t(extproc_pool_t *pool, signal_t *interruptor);

    wget_result_t wget(const std::string &url,
                       const std::vector<std::string> &headers,
                       size_t limit_rate);

    // Marks the extproc worker as errored to simplify cleanup later
    void worker_error();

private:
    static bool worker_fn(read_stream_t *stream_in, write_stream_t *stream_out);

    extproc_job_t extproc_job;
    DISABLE_COPYING(wget_job_t);
};

#endif /* EXTPROC_WGET_JOB_HPP_ */
