// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "extproc/extproc_job.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_worker.hpp"

extproc_job_t::extproc_job_t(extproc_pool_t *_pool,
                             bool (*worker_fn) (read_stream_t *, write_stream_t *),
                             signal_t *_user_interruptor) :
    pool(_pool), user_interruptor(_user_interruptor),
    combined_interruptor(pool->get_shutdown_signal())
{
    if (user_interruptor != NULL) {
        combined_interruptor.add(user_interruptor);
    }

    worker_lock.create(pool->get_worker_semaphore(), &combined_interruptor);

    worker_lock.get()->get_value()->acquired(&combined_interruptor);
    worker_lock.get()->get_value()->run_job(worker_fn);
}

extproc_job_t::~extproc_job_t() {
    assert_thread();
    worker_lock.get()->get_value()->released(user_interruptor);
}

// All data written and read by the user must be accounted for, or things will break
read_stream_t *extproc_job_t::read_stream() {
    assert_thread();
    return worker_lock.get()->get_value()->get_read_stream();
}

write_stream_t *extproc_job_t::write_stream() {
    assert_thread();
    return worker_lock.get()->get_value()->get_write_stream();
}
