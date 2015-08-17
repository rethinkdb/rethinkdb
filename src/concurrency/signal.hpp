// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_SIGNAL_HPP_
#define CONCURRENCY_SIGNAL_HPP_

#include "concurrency/pubsub.hpp"
#include "errors.hpp"

/* A `signal_t` is a boolean variable, combined with a way to be notified if
that boolean variable becomes true. Typically you will construct a concrete
subclass of `signal_t`, then pass a pointer to the underlying `signal_t` to
another object which will read from or listen to it.

To check if a `signal_t` has already been pulsed, call `is_pulsed()`
on it. To be notified when it gets pulsed, construct a
`signal_t::subscription_t` subclass and reset(...) it with the signal to
watch. The callback will be called when the signal is pulsed. If the
signal is already pulsed at the time you construct the
`signal_t::subscription_t`, then the callback will be called
immediately.

`signal_t` is not thread-safe.

Although you may be tempted to, please do not add a method that "unpulses" a
`signal_t`. Part of the definition of a `signal_t` is that it does not return to
the unpulsed state after being pulsed, and some things may depend on that
property. If you want something like that, maybe you should look at something
other than `signal_t`. */

class signal_t : public home_thread_mixin_t {
public:
    /* True if somebody has called `pulse()`. */
    bool is_pulsed() const {
        assert_thread();
        return pulsed;
    }

    /* Crashes if the signal is not pulsed. Note that this works on any thread. This is
    OK because it should only be called if the signal is already pulsed, and if the
    signal is already pulsed, then `pulsed` should not change. */
    void guarantee_pulsed() const {
        guarantee(pulsed);
    }

    /* Wrapper around a `publisher_t<signal_t::subscription_t>::subscription_t`
    */

    class subscription_t {
    public:
        subscription_t() : subs(this) { }
        subscription_t(subscription_t &&movee)
            : subs(std::move(movee.subs)) { }

        virtual ~subscription_t() { }

        void reset(signal_t *s = NULL) {
            if (s) {
                mutex_assertion_t::acq_t acq(&s->lock);
                if (s->is_pulsed()) {
                    subs.subscriber->run();
                } else {
                    subs.reset(s->publisher_controller.get_publisher());
                }
            } else {
                subs.reset(NULL);
            }
        }

        virtual void run() = 0;
    private:
        publisher_t<subscription_t *>::subscription_t subs;
        DISABLE_COPYING(subscription_t);
    };

    /* The coro that calls `wait_lazily_ordered()` will be pushed onto the event
    queue when the signal is pulsed, but will not wake up immediately. Unless
    you really need the ordering guarantee, you should call
    `wait_lazily_unordered()`. */
    void wait_lazily_ordered() const;

    /* The coro that calls `wait_lazily_unordered()` will be notified soon after
    the signal has been pulsed, but not immediately. */
    void wait_lazily_unordered() const;

    /* `wait()` is a deprecated synonym for `wait_lazily_ordered()`. */
    void wait() const {
        wait_lazily_ordered();
    }

    void rethread(threadnum_t new_thread) {
        real_home_thread = new_thread;
        publisher_controller.rethread(new_thread);
        lock.rethread(new_thread);
    }

protected:
    signal_t() : pulsed(false) { }
    signal_t(signal_t &&movee);
    ~signal_t() { reset(); }

    void pulse() THROWS_NOTHING {
        assert_thread();
        mutex_assertion_t::acq_t acq(&lock);
        rassert(!is_pulsed());
        pulsed = true;
        publisher_controller.publish(&signal_t::call);
    }

    void reset();

private:
    static void call(subscription_t *subscription) THROWS_NOTHING {
        subscription->run();
    }

    bool pulsed;
    publisher_controller_t<subscription_t *> publisher_controller;
    mutex_assertion_t lock;
    DISABLE_COPYING(signal_t);
};

#endif /* CONCURRENCY_SIGNAL_HPP_ */
