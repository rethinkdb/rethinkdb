// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef THROTTLING_SEMAPHORE_HPP_
#define THROTTLING_SEMAPHORE_HPP_

#include "containers/scoped.hpp"
#include "concurrency/semaphore.hpp"
#include "concurrency/signal.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/intrusive_list.hpp"
#include "arch/timing.hpp"

/* TODO (daniel): Document
 * It attempts to meet the following design goals:
 * - No lock is ever granted if the given capacity is currently violated (except when using lock_force).
 *   If necessary, lock requests are delayed indefinitely. Note that the capacity requirements
 *   of the issued lock itself are not considered in this guarantee (so no request is
 *   hold back indefinitely if its own count is higher than the capacity).
 * - throttling_semaphore_t does adapt to changing conditions. If locks are released
 *   earlier or later than expected, the delay times of queued lock requests are re-evaluated
 *   automatically.
 * - The configuration of delay times is robust, i.e. throttling_semaphore_t works reasonably well
 *   for a wide range of `delay_at_half` values.
 */
class throttling_semaphore_t : public repeating_timer_callback_t {    
    struct lock_request_t : public intrusive_list_node_t<lock_request_t> {
        semaphore_available_callback_t *cb;
        int count;
        int64_t target_delay;
        int64_t total_time_waited;
        int64_t time_since_recalc;
        int64_t recalc_delay;
        void on_available() {
            cb->on_semaphore_available();
            delete this;
        }
        void progress(int64_t time_passed) {
            total_time_waited += time_passed;            
            time_since_recalc += time_passed;            
        }
    };

    int capacity, current;
    double throttling_threshold; // No throttling if current < capacity * throttling_threshold.
    int64_t delay_granularity;
    int64_t delay_at_half;
    intrusive_list_t<lock_request_t> waiters;
    scoped_ptr_t<repeating_timer_t> timer;
    int64_t last_ring_msecs;
    bool maintain_ordering;


public:
    /* throttling_semaphore_t starts throttling transactions when current >= cap * thre.
     * The granularity in which delays are processed is gran (smaller = higher resolution, more overhead).
     * If current is halfway in between (cap - (cap * thre)) and cap, the delay will be d_half.
     */
    explicit throttling_semaphore_t(int cap, double thre = 0.5, int64_t gran = 20, int64_t d_half = 25) :
        capacity(cap),
        current(0),
        throttling_threshold(thre),
        delay_granularity(gran),
        delay_at_half(d_half),
        maintain_ordering(true)
    {
        rassert(throttling_threshold >= 0.0 && throttling_threshold <= 1.0);
        rassert(delay_granularity > 0);
        rassert(capacity >= 0 || capacity == SEMAPHORE_NO_LIMIT);
    }

    void lock(semaphore_available_callback_t *cb, int count = 1);

    void co_lock(int count = 1);
    void co_lock_interruptible(signal_t *interruptor, int count = 1);

    void unlock(int count = 1);

    void force_lock(int count = 1);

    void set_capacity(int new_capacity);
    int get_capacity() const { return capacity; }
    
    virtual void on_ring(); // Called periodically by timer

private:
    void on_waiters_changed(); // Should be called after a change to waiters
    
    int64_t compute_target_delay() const; // The "heart" of throttling_semaphore_t.
};


class throttling_semaphore_acq_t {
public:
    throttling_semaphore_acq_t(throttling_semaphore_t *_acquiree, int c = 1, bool use_the_force = false) :
                acquiree(_acquiree),
                count(c) {
                    
        if (use_the_force)
            acquiree->force_lock(count);
        else
            acquiree->co_lock(count);
    }

    throttling_semaphore_acq_t(throttling_semaphore_acq_t &&movee) : acquiree(movee.acquiree) {
        movee.acquiree = NULL;
    }

    ~throttling_semaphore_acq_t() {
        if (acquiree) {
            acquiree->unlock(count);
        }
    }

private:
    throttling_semaphore_t *acquiree;
    int count;
    DISABLE_COPYING(throttling_semaphore_acq_t);
};

#endif /* THROTTLING_SEMAPHORE_HPP_ */
