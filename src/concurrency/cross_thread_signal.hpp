#ifndef __CONCURRENCY_CROSS_THREAD_SIGNAL_HPP__
#define __CONCURRENCY_CROSS_THREAD_SIGNAL_HPP__

#include "concurrency/signal.hpp"

/* `cross_thread_signal_t` is useful when you have a `signal_t` on some
thread, but you wish you had the same `signal_t` on a different thread.
Construct a `cross_thread_signal_t` on the original thread, and pass the new
thread to its constructor. It will deliver a notification to the specified
thread when the original signal is pulsed. */

struct cross_thread_signal_t :
    public signal_t,
    private signal_t::waiter_t
{
    cross_thread_signal_t(signal_t *source, int dest_thread) :
        source(source), source_thread(get_thread_id()), dest_thread(dest_thread)
    {
        rassert(source->home_thread == source_thread);
        signal_t::rethread(dest_thread);
        if (source->is_pulsed()) on_signal_pulsed();
        else source->add_waiter(this);
    }

    ~cross_thread_signal_t() {
        rassert(get_thread_id() == source_thread);
        if (!source->is_pulsed()) source->remove_waiter(this);
        drain_semaphore.drain();
        signal_t::rethread(source_thread);
    }

private:
    void on_signal_pulsed() {
        /* This relies on `do_on_thread()` not destroying its function until it
        has returned to the original thread. Since a `drain_semaphore_t::lock_t`
        is one of the arguments to the function, this will keep us alive while
        the notification is delivered. But if `do_on_thread()` destroyed its
        argument while on the other thread, then the `drain_semaphore`'s thread
        requirements would be violated because it should only be accessed from
        one thread. */
        do_on_thread(dest_thread, boost::bind(
            &cross_thread_signal_t::deliver,
            this,
            drain_semaphore_t::lock_t(&drain_semaphore)
            ));
    }

    void deliver(const drain_semaphore_t::lock_t &) {
        rassert(get_thread_id() == dest_thread);
        signal_t::pulse();
    }

    signal_t *source;
    int source_thread, dest_thread;
    drain_semaphore_t drain_semaphore;
};

#endif /* __CONCURRENCY_CROSS_THREAD_SIGNAL_HPP__ */
