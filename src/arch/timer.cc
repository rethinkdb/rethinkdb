// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "arch/timer.hpp"

#include <algorithm>

#include "arch/runtime/thread_pool.hpp"
#include "time.hpp"
#include "utils.hpp"

class timer_token_t : public intrusive_priority_queue_node_t<timer_token_t> {
    friend class timer_handler_t;

private:
    timer_token_t() : interval_nanos(-1), next_time_in_nanos(-1), callback(nullptr) { }

    friend bool left_is_higher_priority(const timer_token_t *left, const timer_token_t *right);

    // The time between rings, if a repeating timer, otherwise zero.
    int64_t interval_nanos;

    // The time of the next 'ring'.
    int64_t next_time_in_nanos;

    // The callback we call upon each 'ring'.
    timer_callback_t *callback;

    DISABLE_COPYING(timer_token_t);
};

bool left_is_higher_priority(const timer_token_t *left, const timer_token_t *right) {
    return left->next_time_in_nanos < right->next_time_in_nanos;
}

timer_handler_t::timer_handler_t(linux_event_queue_t *queue)
    : timer_provider(queue),
      expected_oneshot_time_in_nanos(0) {
    // Right now, we have no tokens.  So we don't ask the timer provider to do anything for us.
}

timer_handler_t::~timer_handler_t() {
    guarantee(token_queue.empty());
}

void timer_handler_t::on_oneshot() {
    // If the timer_provider tends to return its callback a touch early, we don't want to make a
    // bunch of calls to it, returning a tad early over and over again, leading up to a ticks
    // threshold.  So we bump the real time up to the threshold when processing the priority queue.
    int64_t real_ticks = get_ticks().nanos;
    int64_t ticks = std::max(real_ticks, expected_oneshot_time_in_nanos);

    while (!token_queue.empty() && token_queue.peek()->next_time_in_nanos <= ticks) {
        timer_token_t *token = token_queue.pop();

        // Put the repeating timer back on the queue before the callback can be called (so that it
        // may be canceled).
        if (token->interval_nanos != 0) {
            token->next_time_in_nanos = real_ticks + token->interval_nanos;
            token_queue.push(token);
        }

        token->callback->on_timer(ticks_t{real_ticks});

        // Delete nonrepeating timer tokens.
        if (token->interval_nanos == 0) {
            delete token;
        }
    }

    // We've processed young tokens.  Now schedule a new one-shot (if necessary).
    if (!token_queue.empty()) {
        timer_provider.schedule_oneshot(token_queue.peek()->next_time_in_nanos, this);
    }
}

timer_token_t *timer_handler_t::add_timer_internal(
        const ticks_t next_time, const int64_t interval_ms,
        timer_callback_t *callback) {
    rassert(next_time.nanos >= 0);
    rassert(interval_ms >= 0);

    timer_token_t *const token = new timer_token_t;
    token->interval_nanos = interval_ms * MILLION;
    token->next_time_in_nanos = next_time.nanos;
    token->callback = callback;

    const timer_token_t *top_entry = token_queue.peek();
    token_queue.push(token);

    if (top_entry == nullptr || next_time.nanos < top_entry->next_time_in_nanos) {
        timer_provider.schedule_oneshot(next_time.nanos, this);
    }

    return token;
}

void timer_handler_t::cancel_timer(timer_token_t *token) {
    token_queue.remove(token);
    delete token;

    if (token_queue.empty()) {
        timer_provider.unschedule_oneshot();
    }
}



timer_token_t *add_timer2(ticks_t next_time, int64_t interval_ms,
                          timer_callback_t *callback) {
    rassert(interval_ms > 0);
    return linux_thread_pool_t::get_thread()->timer_handler.add_timer_internal(
        next_time, interval_ms, callback);
}

timer_token_t *add_timer(int64_t ms, timer_callback_t *callback) {
    rassert(ms > 0);
    int64_t next_time_in_nanos = get_ticks().nanos + ms * MILLION;
    return linux_thread_pool_t::get_thread()->timer_handler.add_timer_internal(
        ticks_t{next_time_in_nanos}, ms, callback);
}

timer_token_t *fire_timer_once(int64_t ms, timer_callback_t *callback) {
    int64_t next_time_in_nanos = get_ticks().nanos + ms * MILLION;
    return linux_thread_pool_t::get_thread()->timer_handler.add_timer_internal(
        ticks_t{next_time_in_nanos}, 0, callback);
}

void cancel_timer(timer_token_t *timer) {
    linux_thread_pool_t::get_thread()->timer_handler.cancel_timer(timer);
}
