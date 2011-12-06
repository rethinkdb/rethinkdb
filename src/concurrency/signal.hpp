#ifndef __CONCURRENCY_SIGNAL_HPP__
#define __CONCURRENCY_SIGNAL_HPP__

#include "concurrency/pubsub.hpp"
#include "utils.hpp"

/* A `signal_t` is a boolean variable, combined with a way to be notified if
that boolean variable becomes true. Typically you will construct a concrete
subclass of `signal_t`, then pass a pointer to the underlying `signal_t` to
another object which will read from or listen to it.

To check if a `signal_t` has already been pulsed, call `is_pulsed()` on it. To
be notified when it gets pulsed, construct a `signal_t::subscription_t` and pass
a callback function and the signal to watch to its constructor. The callback
will be called when the signal is pulsed.

If you construct a `signal_t::subscription_t` for a signal that's already been
pulsed, you will get an assertion failure.

`signal_t` is generally not thread-safe, although the `wait_*()` functions are.

Although you may be tempted to, please do not add a method that "unpulses" a
`signal_t`. Part of the definition of a `signal_t` is that it does not return to
the unpulsed state after being pulsed, and some things may depend on that
property. If you want something like that, maybe you should look at something
other than `signal_t`; have you tried `resettable_cond_t`? */

class signal_t :
    public home_thread_mixin_t
{
public:
    /* True if somebody has called `pulse()`. */
    bool is_pulsed() const {
        return pulsed;
    }

    /* Wrapper around a `publisher_t<boost::function<void()> >::subscription_t`
    */
    class subscription_t : public home_thread_mixin_t {
    public:
        explicit subscription_t(boost::function<void()> cb) :
            subs(cb) {
        }
        subscription_t(boost::function<void()> cb, signal_t *s) :
            subs(cb, s->publisher_controller.get_publisher()) {
            rassert(!s->is_pulsed());
        }
        void resubscribe(signal_t *s) {
            rassert(!s->is_pulsed());
            subs.resubscribe(s->publisher_controller.get_publisher());
        }
        void unsubscribe() {
            subs.unsubscribe();
        }
    private:
        publisher_t<boost::function<void()> >::subscription_t subs;
        DISABLE_COPYING(subscription_t);
    };

    /* `lock_t` prevents the `signal_t` from being pulsed. It's so that you can
    check the value of a signal and then create a subscriber without worrying
    about something pulsing the signal in between. In theory you can just make
    sure that you never call `coro_t::wait()` between those two things, but it's
    better to enforce it explicitly than to implicitly depend on it. */
    class lock_t {
    public:
        lock_t(signal_t *s) : acq(&s->lock) { }
        void reset() {
            acq.reset();
        }
    private:
        rwi_lock_t::read_acq_t acq;
        DISABLE_COPYING(lock_t);
    };

    /* The coro that calls `wait_lazily_ordered()` will be pushed onto the event
    queue when the signal is pulsed, but will not wake up immediately. Unless
    you really need the ordering guarantee, you should call
    `wait_lazily_unordered()`. */
    void wait_lazily_ordered();

    /* The coro that calls `wait_lazily_unordered()` will be notified soon after
    the signal has been pulsed, but not immediately. */
    void wait_lazily_unordered();

    /* `wait()` is a deprecated synonym for `wait_lazily_ordered()`. */
    void wait() {
        wait_lazily_ordered();
    }

protected:
    explicit signal_t(int specified_home_thread)
        : home_thread_mixin_t(specified_home_thread),
          pulsed(false), publisher_controller(&lock, specified_home_thread) { }
    signal_t() : pulsed(false), publisher_controller(&lock) { }
    ~signal_t() { }

    void pulse() THROWS_NOTHING {
        rwi_lock_t::write_acq_t acq(&lock);
        rassert(!is_pulsed());
        pulsed = true;
        publisher_controller.publish(&signal_t::call, &acq);
    }

private:
    static void call(boost::function<void()>& fun) THROWS_NOTHING {
        fun();
    }

    bool pulsed;
    rwi_lock_t lock;
    publisher_controller_t<boost::function<void()> > publisher_controller;
    DISABLE_COPYING(signal_t);
};

#endif /* __CONCURRENCY_SIGNAL_HPP__ */
