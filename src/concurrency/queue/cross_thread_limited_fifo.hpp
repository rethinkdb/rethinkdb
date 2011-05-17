#ifndef __CONCURRENCY_QUEUE_CROSS_THREAD_LIMITED_FIFO_HPP__
#define __CONCURRENCY_QUEUE_CROSS_THREAD_LIMITED_FIFO_HPP__

#include "concurrency/queue/passive_producer.hpp"
#include "concurrency/semaphore.hpp"
#include <list>

/* `cross_thread_limited_fifo_t` is like `limited_fifo_t`, except that it is
efficient even when objects are being pushed onto the queue from a thread other
than the home thread. In the constructor, pass an extra parameter for the
thread that you intend to push objects onto the queue from. Pushing objects
onto the queue from that thread will be very efficient. */

template<class value_t, class queue_t=std::list<value_t> >
struct cross_thread_limited_fifo_t :
    public passive_producer_t<value_t>,
    public home_thread_mixin_t
{
    cross_thread_limited_fifo_t(int st, int capacity, float trickle_fraction = 0.0) :
        passive_producer_t<value_t>(&available_var),
        source_thread(st),
        semaphore(capacity, trickle_fraction)
    {
        assert_good_thread_id(source_thread);
    }

    void push(const value_t &value) {
        rassert(get_thread_id() == source_thread);
        semaphore.co_lock();
        do_on_thread(home_thread,
            boost::bind(&cross_thread_limited_fifo_t<value_t, queue_t>::do_push, this, value_t(value))
            );
    }

    void set_capacity(int capacity) {
        on_thread_t thread_switcher(source_thread);
        semaphore.set_capacity(capacity);
    }

private:
    int source_thread;
    adjustable_semaphore_t semaphore;
    queue_t queue;
    watchable_var_t<bool> available_var;

    void do_push(const value_t &value) {
        assert_thread();
        queue.push_back(value);
        available_var.set(!queue.empty());
    }

    value_t produce_next_value() {
        assert_thread();
        value_t v = queue.front();
        queue.pop_front();
        do_on_thread(source_thread, boost::bind(&adjustable_semaphore_t::unlock, &semaphore, 1));
        available_var.set(!queue.empty());
        return v;
    }

    DISABLE_COPYING(cross_thread_limited_fifo_t);
};

#endif /* __CONCURRENCY_QUEUE_CROSS_THREAD_LIMITED_FIFO_HPP__ */

