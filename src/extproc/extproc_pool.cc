// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "containers/object_buffer.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_spawner.hpp"

extproc_pool_t::extproc_pool_t(size_t worker_count) :
    ct_interruptors(&interruptor),
    worker_cnt(0),
    prev_worker_cnt(0),
    dealloc_timer(DEALLOC_TIMER_FREQ_MS, this),
    worker_semaphore(worker_count,
                     extproc_spawner_t::get_instance()),
    dealloc_pumper([this](signal_t *interruptor_) { dealloc_blocking(interruptor_); })
    { }

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

void extproc_pool_t::on_ring() {
    dealloc_pumper.notify();
}

void extproc_pool_t::dealloc_blocking(UNUSED signal_t *interruptor_) {
    int cur_worker_cnt = worker_cnt;
    int dealloc_cnt = (prev_worker_cnt - cur_worker_cnt) / 2;
    prev_worker_cnt = cur_worker_cnt;

    for (int i = 0; i < dealloc_cnt; ++i) {
        object_buffer_t<cross_thread_semaphore_t<extproc_worker_t>::lock_t> worker_lock;
        worker_lock.create(get_worker_semaphore(), get_shutdown_signal());

        if (worker_lock.get()->get_value()->is_process_alive()) {
            worker_lock.get()->get_value()->kill_process();
        }
    }
}

void extproc_pool_t::on_worker_acquired()
{
    __sync_add_and_fetch(&worker_cnt, 1);
}

void extproc_pool_t::on_worker_released()
{
    __sync_sub_and_fetch(&worker_cnt, 1);
}

extproc_pool_t::ct_interruptors_t::ct_interruptors_t(signal_t *shutdown_signal) :
    ct_signals(get_num_threads())
{
    for (int i = 0; i < get_num_threads(); ++i) {
        ct_signals[i].init(new cross_thread_signal_t(shutdown_signal, threadnum_t(i)));
    }
}

signal_t *extproc_pool_t::ct_interruptors_t::get() {
    return ct_signals[get_thread_id().threadnum].get();
}
