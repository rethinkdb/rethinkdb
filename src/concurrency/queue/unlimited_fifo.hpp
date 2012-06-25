#ifndef CONCURRENCY_QUEUE_UNLIMITED_FIFO_HPP_
#define CONCURRENCY_QUEUE_UNLIMITED_FIFO_HPP_

#include <list>

#include "concurrency/queue/passive_producer.hpp"
#include "perfmon/perfmon.hpp"

/* `unlimited_fifo_queue_t` is one of the simplest possible implementations of
`passive_producer_t`. It's a first-in, first-out queue. It's called "unlimited"
to emphasize that it can grow to an arbitrary size, which could be dangerous.

It's templated on an underlying data structure so that you can use an
`intrusive_list_t` or something like that if you prefer. */

namespace unlimited_fifo_queue {
template <class T>
T get_front_of_list(std::list<T>& list) {
    return list.front();
}

template <class T>
T *get_front_of_list(intrusive_list_t<T>& list) {
    return list.head();
}

}  // unlimited_fifo_queue

template<class value_t, class queue_t = std::list<value_t> >
struct unlimited_fifo_queue_t : public passive_producer_t<value_t> {
    unlimited_fifo_queue_t() 
        : passive_producer_t<value_t>(&available_control),
          counter(NULL)
    { }

    explicit unlimited_fifo_queue_t(perfmon_counter_t *_counter)
        : passive_producer_t<value_t>(&available_control),
          counter(_counter)
    { }

    void push(const value_t& value) {
        if (counter) {
            ++(*counter);
        }

        queue.push_back(value);
        available_control.set_available(!queue.empty());
    }

    int size() {
        return queue.size();
    }

private:
    availability_control_t available_control;
    value_t produce_next_value() {
        if (counter) {
            --(*counter);
        }

        value_t v = unlimited_fifo_queue::get_front_of_list(queue);
        queue.pop_front();
        available_control.set_available(!queue.empty());
        return v;
    }
    queue_t queue;
    perfmon_counter_t *counter;
    DISABLE_COPYING(unlimited_fifo_queue_t);
};

#endif /* CONCURRENCY_QUEUE_UNLIMITED_FIFO_HPP_ */
