#include "clustering/immediate_consistency/branch/backfill_throttler.hpp"

#include "concurrency/cross_thread_signal.hpp"

backfill_throttler_t::backfill_throttler_t() :
    global_sem(GLOBAL_LIMIT) { }

backfill_throttler_t::lock_t::lock_t(backfill_throttler_t *p, signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) :
    parent(p) {

    cross_thread_signal_t ct_interruptor(interruptor, parent->home_thread());
    {
        on_thread_t th(parent->home_thread());
        global_acq.init(&parent->global_sem, 1);
        wait_interruptible(global_acq.acquisition_signal(), &ct_interruptor);
    }
}

backfill_throttler_t::lock_t::~lock_t() {
    on_thread_t th(parent->home_thread());
    global_acq.reset();
}
