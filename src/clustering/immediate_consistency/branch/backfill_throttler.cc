#include "clustering/immediate_consistency/branch/backfill_throttler.hpp"

#include "concurrency/cross_thread_signal.hpp"

backfill_throttler_t::backfill_throttler_t() :
    global_sem(GLOBAL_LIMIT) { }

backfill_throttler_t::lock_t::lock_t(backfill_throttler_t *p,
                                     peer_id_t from_peer,
                                     signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) :
    parent(p) {

    cross_thread_signal_t ct_interruptor(interruptor, parent->home_thread());
    {
        on_thread_t th(parent->home_thread());
        try {
            // Acquire first the peer semaphore and then the global one.
            // That makes sure that we don't lock out backfills on other peers
            // because the semaphore on our peer is currently exhausted.
            auto peer_sem_it = parent->peer_sems.find(from_peer);
            if (peer_sem_it == parent->peer_sems.end()) {
                peer_sem_it = parent->peer_sems.insert(from_peer,
                        new new_semaphore_t(parent->PER_PEER_LIMIT)).first;
            }
            peer_acq.init(new new_semaphore_acq_t(peer_sem_it->second, 1));
            wait_interruptible(peer_acq->acquisition_signal(), &ct_interruptor);
            global_acq.init(new new_semaphore_acq_t(&parent->global_sem, 1));
            wait_interruptible(global_acq->acquisition_signal(), &ct_interruptor);
        } catch (interrupted_exc_t &e) {
            // Be careful to destroy the acquisitions on the right thread.
            reset();
            throw;
        }
    }
}

void backfill_throttler_t::lock_t::reset() {
    if (global_acq.has()) {
        global_acq.reset();
    }
    if (peer_acq.has()) {
        peer_acq.reset();
    }
}

backfill_throttler_t::lock_t::~lock_t() {
    on_thread_t th(parent->home_thread());
    reset();
}
