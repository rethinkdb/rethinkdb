#ifndef __CONCURRENCY_QUEUE_UNLIMITED_FIFO_HPP__
#define __CONCURRENCY_QUEUE_UNLIMITED_FIFO_HPP__

#include "concurrency/queue/passive_producer.hpp"
#include <list>

/* `unlimited_fifo_queue_t` is one of the simplest possible implementations of
`passive_producer_t`. It's a first-in, first-out queue. It's called "unlimited"
to emphasize that it can grow to an arbitrary size, which could be dangerous.

It's templated on an underlying data structure so that you can use an
`intrusive_list_t` or something like that if you prefer. */

template<class value_t, class queue_t = std::list<value_t> >
struct unlimited_fifo_queue_t : public passive_producer_t<value_t> {

    unlimited_fifo_queue_t() :
        passive_producer_t<value_t>(&available_var) { }

    void push(const value_t &value) {
        queue.push_back(value);
        available_var.set(!queue.empty());
    }

private:
    watchable_var_t<bool> available_var;
    value_t produce_next_value() {
        value_t v = queue.front();
        queue.pop_front();
        available_var.set(!queue.empty());
        return v;
    }
    queue_t queue;
    DISABLE_COPYING(unlimited_fifo_queue_t);
};

#endif /* __CONCURRENCY_QUEUE_UNLIMITED_FIFO_HPP__ */
