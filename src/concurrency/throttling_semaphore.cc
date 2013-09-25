// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "concurrency/throttling_semaphore.hpp"

#include "arch/runtime/coroutines.hpp"
#include "concurrency/cond_var.hpp"
#include "utils.hpp"


void throttling_semaphore_t::on_waiters_changed() {
    // Start or stop the timer depending on whether waiters is empty or not
    if (waiters.empty() && timer.has()) {
        timer.reset();
    } else if (!waiters.empty()) {
        if (!timer.has()) {
            // Starting a fresh timer
            const double secs = ticks_to_secs(get_ticks());
            const int64_t msecs = static_cast<int64_t>(secs * 1000.0);
            last_ring_msecs = msecs;
        }
        // The next wakeup is either delay_granularity, or the next request to complete its target delay.
        // Whichever comes first.
        int64_t next_wakeup = delay_granularity;
        lock_request_t *request = waiters.head();
        while (request != NULL) {
            int64_t time_till_delay_complete = std::max((int64_t)1, request->target_delay - request->total_time_waited);
            next_wakeup = std::min(next_wakeup, time_till_delay_complete);
            if (maintain_ordering) break;
            request = waiters.next(request);
        }
        timer.init(new repeating_timer_t(next_wakeup, this));
    }
}

void throttling_semaphore_t::on_ring() {    
    const double secs = ticks_to_secs(get_ticks());
    const int64_t msecs = static_cast<int64_t>(secs * 1000.0);
    const int64_t time_passed = msecs - last_ring_msecs;
    last_ring_msecs = msecs;
    
    lock_request_t *request = waiters.head();
    while (request != NULL) {
        lock_request_t *next_request = waiters.next(request);
        
        request->progress(time_passed);
        
        const bool might_have_waited_long_enough = request->total_time_waited >= request->target_delay;
        const bool recalc_delay_met = request->time_since_recalc >= request->recalc_delay;
        if (recalc_delay_met || might_have_waited_long_enough) {
            request->target_delay = compute_target_delay();
            // Wait longer before we perform the next re-calculation
            request->recalc_delay *= 2; // TODO (daniel): Should we limit this to some reasonable value?
        }
        
        const bool has_waited_long_enough = request->total_time_waited >= request->target_delay;
        if (has_waited_long_enough) {
            current += request->count;
            waiters.remove(request);
            request->on_available();
        } else {
            // If we want to maintain the ordering of requests,
            // we must *not* skip ahead and accept a later request first.
            if (maintain_ordering) break;
        }
        
        request = next_request;
    }
    on_waiters_changed();
}

void throttling_semaphore_t::lock(semaphore_available_callback_t *cb, int count) {
    int64_t target_delay = compute_target_delay();
    if (target_delay == 0) {
        // Continue immediately
        current += count;
        cb->on_semaphore_available();
    } else {
        // Put it on the queue
        lock_request_t *r = new lock_request_t;
        r->count = count;
        r->cb = cb;
        r->recalc_delay = delay_granularity; // Re-calculate the delay for the first time at delay_granularity
        r->target_delay = target_delay;
        r->time_since_recalc = 0;
        r->total_time_waited = 0;
        waiters.push_back(r);
        on_waiters_changed();
    }
}

void throttling_semaphore_t::unlock(int count) {
    rassert(current >= count);
    current -= count;
}

void throttling_semaphore_t::force_lock(int count) {
    current += count;
}

void throttling_semaphore_t::set_capacity(int new_capacity) {
    capacity = new_capacity;
}

void throttling_semaphore_t::co_lock(int count) {
    struct : public semaphore_available_callback_t, public one_waiter_cond_t {
        void on_semaphore_available() { pulse(); }
    } cb;
    lock(&cb, count);
    cb.wait_eagerly_deprecated();
    coro_t::yield();
}

void throttling_semaphore_t::co_lock_interruptible(signal_t *interruptor, int count) {
    struct : public semaphore_available_callback_t, public cond_t {
        void on_semaphore_available() { pulse(); }
    } cb;
    lock(&cb, count);

    try {
        wait_interruptible(&cb, interruptor);
    } catch (const interrupted_exc_t& ex) {
        // Remove our lock request from the queue
        for (lock_request_t *request = waiters.head(); request != NULL; request = waiters.next(request)) {
            if (request->cb == &cb) {
                waiters.remove(request);
                on_waiters_changed();
                delete request;
                break;
            }
        }
        throw;
    }
}

int64_t throttling_semaphore_t::compute_target_delay() const {
    // Do not delay if we are below the throttling_threshold
    const int64_t threshold = static_cast<int64_t>(static_cast<double>(capacity) * throttling_threshold);
    if (current < threshold) return 0;
    
    rassert(current > threshold);
    rassert(threshold < capacity);
    const double delay_level = std::min(1.0, static_cast<double>(current - threshold) / static_cast<double>(capacity - threshold));
    rassert(delay_level >= 0.0 && delay_level <= 1.0);
    // This is the function we are using to calculate delays: http://www.wolframalpha.com/input/?i=1%2F%281-x%29+-+1+for+x+from+0+to+1
    // If necessary, the steepness can be adjusted by taking a power of delay_level. This
    // is not currently implemented here though.
    if (delay_level > 0.999999) // Fall-through for (close enough to) infinite
        return 1000000;
    // We multiply the value of the function by a delay_at_half constant.
    // delay_at_half is the delay in ms when delay_level = 0.5
    const double target_delay = static_cast<double>(delay_at_half) * (1.0/(1.0 - delay_level) - 1.0);
    return static_cast<int64_t>(target_delay);
}
