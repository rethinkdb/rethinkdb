#ifndef __CONCURRENCY_CROSS_THREAD_SIGNAL_HPP__
#define __CONCURRENCY_CROSS_THREAD_SIGNAL_HPP__

#include "concurrency/signal.hpp"
#include "concurrency/drain_semaphore.hpp"

/* `cross_thread_signal_t` is useful when you have a `signal_t` on some
thread, but you wish you had the same `signal_t` on a different thread.
Construct a `cross_thread_signal_t` on the original thread, and pass the new
thread to its constructor. It will deliver a notification to the specified
thread when the original signal is pulsed. */

class cross_thread_signal_t :
    public signal_t,
    private signal_t::waiter_t
{
public:
    cross_thread_signal_t(signal_t *source, int dest_thread);
    ~cross_thread_signal_t();

private:
    void on_signal_pulsed();

    void deliver(const drain_semaphore_t::lock_t &);

    signal_t *source;
    int source_thread, dest_thread;
    drain_semaphore_t drain_semaphore;
};

#endif /* __CONCURRENCY_CROSS_THREAD_SIGNAL_HPP__ */
