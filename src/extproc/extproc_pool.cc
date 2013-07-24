// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_worker.hpp"
#include "extproc/extproc_spawner.hpp"
#include "extproc/extproc_job.hpp"

extproc_pool_t::extproc_pool_t(size_t worker_count) :
    workers(worker_count),
    worker_semaphore(worker_count)
{
    // The spawner should have been constructed outside of the thread pool
    extproc_spawner_t *spawner = extproc_spawner_t::get_instance();
    guarantee(spawner != NULL);

    // Construct cross-thread signals off of our interruptor for the workers
    ct_interruptors.init(get_num_threads());
    for (int i = 0; i < get_num_threads(); ++i) {
        ct_interruptors[i].init(new cross_thread_signal_t(&interruptor, i));
    }

    for (size_t i = 0; i < workers.size(); ++i) {
        workers[i].exchange(new extproc_worker_t(&ct_interruptors, spawner));
    }
}

extproc_pool_t::~extproc_pool_t() {
    // Can only be destructed on the same thread we were created on
    assert_thread();

    // Interrupt any and all workers
    interruptor.pulse();

    // Lock all workers, running jobs should have been cancelled by the interruptor
    for (size_t i = 0; i < workers.size(); ++i) {
        worker_semaphore.lock(NULL);
    }

    // Verify that all workers are back and release them
    for (size_t i = 0; i < workers.size(); ++i) {
        extproc_worker_t *worker = workers[i].load();
        guarantee(worker != NULL);
        delete worker;
    }
}

extproc_worker_t *extproc_pool_t::acquire_worker(size_t *index, signal_t *user_interruptor) {
    // Take a worker from the semaphore (blocking)
    worker_semaphore.lock(user_interruptor);

    // Loop over worker table and take one
    for (size_t i = 0; i < workers.size(); ++i) {
        extproc_worker_t *selected_worker;
        selected_worker = workers[i].exchange(NULL);
        if (selected_worker != NULL) {
            *index = i;
            selected_worker->acquired(user_interruptor);
            return selected_worker;
        }
    }

    // We should never get here, it would mean there's a problem with our semaphore
    unreachable();
}

// Release will put the worker back into the table at index, and release an item in
// the eventfd semaphore
void extproc_pool_t::release_worker(extproc_worker_t *worker, size_t index) {
    guarantee(index < workers.size());

    // Clear the old user's interruptor
    worker->released();

    // Put the worker back in its index
    extproc_worker_t *old_val = workers[index].exchange(worker);

    // Sanity check
    guarantee(old_val == NULL);

    // Release the worker to the semaphore
    worker_semaphore.unlock();
}
