#ifndef __CONCURRENCY_SIGNAL_HPP__
#define __CONCURRENCY_SIGNAL_HPP__

#include "arch/coroutines.hpp"
#include "containers/intrusive_list.hpp"
#include "utils.hpp"

/* A signal_t is a boolean variable, combined with a way to be notified if that boolean
variable is pulsed. Typically you will construct a concrete subclass of signal_t, then pass
a pointer to the underlying signal_t to another object which will wait on it.

`signal_t` is generally not thread-safe, although the `wait_*()` functions are.

Although you may be tempted to, please do not add a method that "unpulses" a signal_t. Part of
the definition of a signal_t is that it does not return to the unpulsed state after being
pulsed, and some things may depend on that property. If you want something like that, maybe you
should look at something other than signal_t; have you tried resettable_cond_t? */

struct signal_t : public home_thread_mixin_t {

public:
    /* Waiters are always notified on the `signal_t`'s home thread. */
    struct waiter_t : public intrusive_list_node_t<waiter_t> {
        virtual void on_signal_pulsed() = 0;
    protected:
        virtual ~waiter_t() { }
    };

    /* It is illegal to add a waiter to a signal when it is in a call to `pulse()`,
    but it is legal to remove a waiter if that waiter hasn't been notified yet. */
    void add_waiter(waiter_t *);
    void remove_waiter(waiter_t *);

    // Means that somebody has called pulse().
    bool is_pulsed() const;

    /* The coro that calls `wait_lazily()` will be pushed onto the event queue
    when the signal is pulsed, but will not wake up immediately. */
    void wait_lazily() {
        on_thread_t thread_switcher(home_thread());
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

    /* The coro that calls `wait_eagerly()` will be woken up immediately when
    the signal is pulsed, before `pulse()` even returns. */
    void wait_eagerly() {
        on_thread_t thread_switcher(home_thread());
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

    /* `wait()` is a synonym for `wait_lazily()`, but it's better to explicitly
    call `wait_lazily()` or `wait_eagerly()`. */
    void wait() {
        wait_lazily();
    }

    void rethread(int new_thread) {
        rassert(waiters.empty(), "It might not be safe to rethread() a signal_t with "
            "something currently waiting on it.");
        real_home_thread = new_thread;
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
};

#endif /* __CONCURRENCY_SIGNAL_HPP__ */
