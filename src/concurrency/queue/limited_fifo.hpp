#ifndef __CONCURRENCY_QUEUE_LIMITED_FIFO_HPP__
#define __CONCURRENCY_QUEUE_LIMITED_FIFO_HPP__

#include "concurrency/queue/passive_producer.hpp"
#include "concurrency/semaphore.hpp"
#include <list>

/* `limited_fifo_queue_t` is a first-in, first-out queue that has a limited depth. If the
consumer is not reading of the queue as fast as the producer is pushing things onto the
queue, then `push()` will start to block. */

template<class value_t, class queue_t = std::list<value_t> >
struct limited_fifo_queue_t : public passive_producer_t<value_t> {

    limited_fifo_queue_t(int depth) :
        passive_producer_t(&available_var),
        semaphore(depth)
    {
        rassert(depth > 0);
    }

    void push(const value_t &value) {
        semaphore.lock();
        queue.push_back(value);
        available_var.set(!queue.empty());
    }

private:
    semaphore_t semaphore;
    watchable_var_t<bool> available_var;
    value_t produce_next_value() {
        value_t v = queue.front();
        queue.pop_front();
        semaphore.unlock();
        available_var.set(!queue.empty());
        return v;
    }
    queue_t queue;
    DISABLE_COPYING(limited_fifo_queue_t);
};

#endif /* __CONCURRENCY_QUEUE_LIMITED_FIFO_HPP__ */
