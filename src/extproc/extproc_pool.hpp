// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef EXTPROC_EXTPROC_POOL_HPP_
#define EXTPROC_EXTPROC_POOL_HPP_

#include "utils.hpp"
#include "containers/scoped.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_semaphore.hpp"
#include "extproc/extproc_worker.hpp"

// Extproc pool is used to acquire and release workers from any thread,
//  must be created from within the thread pool
class extproc_pool_t : public home_thread_mixin_t {
public:
    explicit extproc_pool_t(size_t worker_count);
    ~extproc_pool_t();

    // Get the signal for the current thread that will indicate when this object is being
    //  destroyed, make sure to combine with any other interruptors or shutdown may hang
    signal_t *get_shutdown_signal();

    // Get the semaphore of workers to obtain a lock (may be done from any thread)
    cross_thread_semaphore_t<extproc_worker_t> *get_worker_semaphore();

private:
    // The interruptor to be pulsed when shutting down
    cond_t interruptor;

    // Crossthreaded interruptors to notify workers on any thread when we are shutting down
    //  (this is its own class so we can initialize everything in the initializer list)
    class ct_interruptors_t {
    public:
        explicit ct_interruptors_t(signal_t *shutdown_signal);
        signal_t *get();
    private:
        scoped_array_t<scoped_ptr_t<cross_thread_signal_t> > ct_signals;
    } ct_interruptors;

    // Cross-threaded semaphore allowing workers to be acquired from any thread
    cross_thread_semaphore_t<extproc_worker_t> worker_semaphore;
};

#endif /* EXTPROC_EXTPROC_POOL_HPP_ */
