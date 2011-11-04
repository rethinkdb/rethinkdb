#ifndef __CONCURRENCY_CROSS_THREAD_SIGNAL_HPP__
#define __CONCURRENCY_CROSS_THREAD_SIGNAL_HPP__

#include "concurrency/signal.hpp"
#include "concurrency/auto_drainer.hpp"

/* `cross_thread_signal_t` is useful when you have a `signal_t` on some
thread, but you wish you had the same `signal_t` on a different thread.
Construct a `cross_thread_signal_t` on the original thread, and pass the new
thread to its constructor. It will deliver a notification to the specified
thread when the original signal is pulsed. */

class cross_thread_signal_t :
    public signal_t
{
public:
    cross_thread_signal_t(signal_t *source, int dest_thread);

private:
    void on_signal_pulsed(auto_drainer_t::lock_t);
    void deliver(auto_drainer_t::lock_t);

    int source_thread, dest_thread;

    /* This object's constructor rethreads our internal `signal_t` to our other
    thread, and then reverses it in the destructor. It must be a separate object
    instead of logic in the constructor/destructor because its destructor must
    be run after `drainer`'s destructor. */
    class rethreader_t {
    public:
        rethreader_t(signal_t *s, int d) :
            signal(s), original(signal->home_thread())
        {
            signal->rethread(d);
        }
        ~rethreader_t() {
            signal->rethread(original);
        }
    private:
        signal_t *signal;
        int original;
    } rethreader;

    /* `drainer` makes sure we don't shut down with a signal still in flight */
    auto_drainer_t drainer;

    /* The destructor for `subs` must be run before the destructor for `drainer`
    because `drainer`'s destructor will block until all the
    `auto_drainer_t::lock_t` objects are gone, and `subs`'s callback holds an
    `auto_drainer_t::lock_t`. */
    signal_t::subscription_t subs;
};

#endif /* __CONCURRENCY_CROSS_THREAD_SIGNAL_HPP__ */
