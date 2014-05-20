#include "clustering/immediate_consistency/branch/backfill_throttler.hpp"

#include "arch/runtime/runtime_utils.hpp"
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
        boost::ptr_map<peer_id_t, new_semaphore_t>::iterator peer_sem_it;
        {
            ASSERT_NO_CORO_WAITING;
            // Atomically get the peer_sem and increment the reference count
            peer_sem_it = parent->peer_sems.find(from_peer);
            if (peer_sem_it == parent->peer_sems.end()) {
                peer_sem_it = parent->peer_sems.insert(from_peer,
                        new new_semaphore_t(parent->PER_PEER_LIMIT)).first;
            }
            ++parent->peer_sems_refcount[from_peer];
            peer.reset(from_peer);
        }
        try {
            // Acquire first the peer semaphore and then the global one.
            // That makes sure that we don't lock out backfills on other peers
            // because the semaphore on our peer is currently exhausted.
            peer_acq.init(new new_semaphore_acq_t(peer_sem_it->second, 1));
            wait_interruptible(peer_acq->acquisition_signal(), &ct_interruptor);
            global_acq.init(new new_semaphore_acq_t(&parent->global_sem, 1));
            wait_interruptible(global_acq->acquisition_signal(), &ct_interruptor);
        } catch (const interrupted_exc_t &e) {
            // Destroy the acquisitions on the right thread.
            // If we throw from the constructor, the ~lock_t() destructor is never
            // called. So the acquisitions would be default destructed - possibly on
            // the wrong thread.
            reset();
            throw;
        } catch (...) {
            crash("Encountered an unexpected exception in " \
                  "backfill_throttler_t::lock_t::lock_t().");
        }
    }
}

void backfill_throttler_t::lock_t::reset() {
    parent->assert_thread();
    if (global_acq.has()) {
        global_acq.reset();
    }
    if (peer_acq.has()) {
        peer_acq.reset();
    }
    {
        ASSERT_NO_CORO_WAITING;
        if (peer) {
            rassert(parent->peer_sems_refcount[peer.get()] >= 1);
            --parent->peer_sems_refcount[peer.get()];
            // Check if we can evict the peer semaphore
            if (parent->peer_sems_refcount[peer.get()] == 0) {
                parent->peer_sems_refcount.erase(peer.get());
                parent->peer_sems.erase(peer.get());
            }
            peer.reset();
        }
    }
}

backfill_throttler_t::lock_t::~lock_t() {
    on_thread_t th(parent->home_thread());
    reset();
}
