#ifndef __CONCURRENCY_SIGNAL_HPP__
#define __CONCURRENCY_SIGNAL_HPP__

#include "concurrency/pubsub.hpp"
#include "utils.hpp"
#include <boost/bind.hpp>

/* A `signal_t` is a boolean variable, combined with a way to be notified if
that boolean variable becomes true. Typically you will construct a concrete
subclass of `signal_t`, then pass a pointer to the underlying `signal_t` to
another object which will read from or listen to it.

To check if a `signal_t` has already been pulsed, call `is_pulsed()` on it. To
be notified when it gets pulsed, construct a `signal_t::subscription_t` and pass
a callback function and the signal to watch to its constructor. The callback
will be called when the signal is pulsed.

If you construct a `signal_t::subscription_t` for a signal that's already been
pulsed, nothing will happen.

`signal_t` is generally not thread-safe, although the `wait_*()` functions are.

Although you may be tempted to, please do not add a method that "unpulses" a
`signal_t`. Part of the definition of a `signal_t` is that it does not return to
the unpulsed state after being pulsed, and some things may depend on that
property. If you want something like that, maybe you should look at something
other than `signal_t`; have you tried `resettable_cond_t`? */

struct signal_t :
    /* This is less-than-ideal because it allows subclasses of `signal_t` to
    access `publish()`. They should be calling `pulse()` instead of calling
    `publish()` directly. */
    public publisher_t<boost::function<void()> >
{
public:
    /* True if somebody has called `pulse()`. */
    bool is_pulsed() const {
        return state == state_pulsing || state == state_pulsed;
    }

    /* The coro that calls `wait_lazily()` will be pushed onto the event queue
    when the signal is pulsed, but will not wake up immediately. */
    void wait_lazily() {
        if (!is_pulsed()) {
            subscription_t subs(
                boost::bind(&coro_t::notify_later, coro_t::self()),
                this);
            coro_t::wait();
        }
    }

    /* The coro that calls `wait_eagerly()` will be woken up immediately when
    the signal is pulsed, before `pulse()` even returns. */
    void wait_eagerly() {
        if (!is_pulsed()) {
            subscription_t subs(
                boost::bind(&coro_t::notify_now, coro_t::self()),
                this);
            coro_t::wait();
        }
    }

    /* `wait()` is a synonym for `wait_lazily()`, but it's better to explicitly
    call `wait_lazily()` or `wait_eagerly()`. */
    void wait() {
        wait_lazily();
    }

protected:
    signal_t() : state(state_unpulsed) { }
    ~signal_t() { }

    static void call(const boost::function<void()> &fun) {
        fun();
    }

    void pulse() {
        rassert(state == state_unpulsed);
        state = state_pulsing;
        publish(&signal_t::call);
        state = state_pulsed;
    }

private:
    enum state_t {
        state_unpulsed,
        /* `state_pulsing` means we are *in* the call to `pulse()`. */
        state_pulsing,
        state_pulsed
    } state;

    DISABLE_COPYING(signal_t);
};

#endif /* __CONCURRENCY_SIGNAL_HPP__ */
