// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "extproc/extproc_job.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_worker.hpp"

extproc_job_t::extproc_job_t(extproc_pool_t *_pool,
                             bool (*worker_fn) (read_stream_t *, write_stream_t *),
                             signal_t *interruptor) :
    pool(_pool) {
    worker = pool->acquire_worker(interruptor);
    worker->run_job(worker_fn);
}

extproc_job_t::~extproc_job_t() {
    assert_thread();
    pool->release_worker(worker);
}

// All data written and read by the user must be accounted for, or things will break
read_stream_t *extproc_job_t::read_stream() {
    assert_thread();
    return worker->get_read_stream();
}

write_stream_t *extproc_job_t::write_stream() {
    assert_thread();
    return worker->get_write_stream();
}
