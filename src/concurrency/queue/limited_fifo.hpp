// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_QUEUE_LIMITED_FIFO_HPP_
#define CONCURRENCY_QUEUE_LIMITED_FIFO_HPP_

#include <list>

#include "concurrency/queue/passive_producer.hpp"
#include "concurrency/semaphore.hpp"
#include "perfmon/types.hpp"

/* `limited_fifo_queue_t` is a first-in, first-out queue that has a limited
depth. If the consumer is not reading off the queue as fast as the producer is
pushing things onto the queue, then `push()` will start to block.

The `capacity` and `trickle_fraction` arguments, and the `set_capacity()`
method, work as in `adjustable_semaphore_t`. */

template<class value_t, class queue_t = std::list<value_t> >
struct limited_fifo_queue_t :
    public home_thread_mixin_debug_only_t,
    public passive_producer_t<value_t> {

    limited_fifo_queue_t(
        int capacity, double trickle_fraction = 0.0, perfmon_counter_t *_counter = nullptr)
        : passive_producer_t<value_t>(&available_control),
          semaphore(capacity, trickle_fraction),
          counter(_counter) { }

    void push(const value_t &value) {
        assert_thread();
        if (counter) ++(*counter);
        semaphore.co_lock();
        queue.push_back(value);
        available_control.set_available(!queue.empty());
    }

    void set_capacity(int capacity) {
        assert_thread();
        semaphore.set_capacity(capacity);
    }

private:
    adjustable_semaphore_t semaphore;
    availability_control_t available_control;

    perfmon_counter_t *counter;

    value_t produce_next_value() {
        assert_thread();
        value_t v = queue.front();
        queue.pop_front();
        semaphore.unlock();
        if (counter) --(*counter);
        available_control.set_available(!queue.empty());
        return v;
    }

    queue_t queue;

    DISABLE_COPYING(limited_fifo_queue_t);
};

#endif /* CONCURRENCY_QUEUE_LIMITED_FIFO_HPP_ */
