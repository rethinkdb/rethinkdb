// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_spawner.hpp"

extproc_pool_t::extproc_pool_t(size_t worker_count) :
    ct_interruptors(&interruptor),
    worker_semaphore(worker_count,
                     extproc_spawner_t::get_instance()) { }

extproc_pool_t::~extproc_pool_t() {
    // Can only be destructed on the same thread we were created on
    assert_thread();

    // Interrupt any and all workers
    interruptor.pulse();
}

cross_thread_semaphore_t<extproc_worker_t> *extproc_pool_t::get_worker_semaphore() {
    return &worker_semaphore;
}

signal_t *extproc_pool_t::get_shutdown_signal() {
    return ct_interruptors.get();
}

extproc_pool_t::ct_interruptors_t::ct_interruptors_t(signal_t *shutdown_signal) :
    ct_signals(get_num_threads())
{
    for (int i = 0; i < get_num_threads(); ++i) {
        ct_signals[i].init(new cross_thread_signal_t(shutdown_signal, i));
    }
}

signal_t *extproc_pool_t::ct_interruptors_t::get() {
    return ct_signals[get_thread_id()].get();
}
