#ifndef __CONCURRENCY_SIGNAL_HPP__
#define __CONCURRENCY_SIGNAL_HPP__

#include "containers/intrusive_list.hpp"
#include "utils.hpp"

/* A signal_t is a boolean variable, combined with a way to be notified if that boolean
variable is pulsed. Typically you will construct a concrete subclass of signal_t, then pass
a pointer to the underlying signal_t to another object which will wait on it.

signal_t is not thread-safe.

Although you may be tempted to, please do not add a method that "unpulses" a signal_t. Part of
the definition of a signal_t is that it does not return to the unpulsed state after being
pulsed, and some things may depend on that property. If you want something like that, maybe you
should look at something other than signal_t; have you tried resettable_cond_t? */

struct signal_t : public home_thread_mixin_t {

public:
    struct waiter_t : public intrusive_list_node_t<waiter_t> {
        virtual void on_signal_pulsed() = 0;
    protected:
        virtual ~waiter_t() { }
    };

    void add_waiter(waiter_t *);
    void remove_waiter(waiter_t *);
    bool is_pulsed();

    void wait() {
        if (!is_pulsed()) {
            struct : public waiter_t {
                coro_t *to_wake;
                void on_signal_pulsed() { to_wake->notify(); }
            } waiter;
            waiter.to_wake = coro_t::self();
            add_waiter(&waiter);
            coro_t::wait();
        }
    }

    void wait_eagerly() {
        if (!is_pulsed()) {
            struct : public waiter_t {
                coro_t *to_wake;
                void on_signal_pulsed() { to_wake->notify_now(); }
            } waiter;
            waiter.to_wake = coro_t::self();
            add_waiter(&waiter);
            coro_t::wait();
        }
    }

protected:
    signal_t();
    ~signal_t();
    void pulse();

private:
    DISABLE_COPYING(signal_t);

    intrusive_list_t<waiter_t> waiters;

    enum state_t {
        state_unpulsed,
        state_pulsing,   // We are *in* the call to pulse()
        state_pulsed
    } state;

    bool *set_true_on_death;
};

#endif /* __CONCURRENCY_SIGNAL_HPP__ */
