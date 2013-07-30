// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef EXTPROC_EXTPROC_POOL_HPP_
#define EXTPROC_EXTPROC_POOL_HPP_

#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 4)
#include <cstdatomic>
#else
#include <atomic>
#endif

#include "utils.hpp"
#include "containers/scoped.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_semaphore.hpp"

class extproc_worker_t;

// Extproc pool is used to acquire and release workers from any thread,
//  must be created from within the thread pool
class extproc_pool_t : public home_thread_mixin_t {
public:
    explicit extproc_pool_t(size_t worker_count);
    ~extproc_pool_t();

private:
    // These member functions are only usable by jobs - construct a job to get a worker
    friend class extproc_job_t;

    // Acquire will lock a worker using the eventfd semaphore, and take one from the
    // worker table and replace it with NULL, atomically
    extproc_worker_t *acquire_worker(size_t *index, signal_t *interruptor);

    // Release will put the worker back into the table at index, and release an item in
    // the eventfd semaphore
    void release_worker(extproc_worker_t *worker, size_t index);

    // Pointers to all workers available in this instance, when a worker is taken, it
    //  will be replaced by NULL in the array, and replaced when done
    scoped_array_t<std::atomic<extproc_worker_t *> > workers;

    // Semaphore to control access to workers, when it is locked, there is guaranteed to
    //  be a worker available
    cross_thread_semaphore_t worker_semaphore;

    // The interruptor to be pulsed when shutting down
    cond_t interruptor;

    // Crossthreaded interruptors to notify workers on any thread when we are shutting down
    scoped_array_t<scoped_ptr_t<cross_thread_signal_t> > ct_interruptors;
};

#endif /* EXTPROC_EXTPROC_POOL_HPP_ */
