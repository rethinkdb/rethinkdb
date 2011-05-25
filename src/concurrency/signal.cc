#include "concurrency/signal.hpp"

void signal_t::add_waiter(waiter_t *w) {
    assert_thread();
    switch (state) {
        case state_unpulsed:
            waiters.push_back(w);
            break;
        case state_pulsing:
            crash("Trying to add a waiter to a signal while it is being pulsed probably "
                "indicates a programmer error.");
        case state_pulsed:
            crash("Trying to add a waiter to a signal after it has been pulsed probably "
                "indicates a programmer error.");
        default:
            unreachable();
    }
}

void signal_t::remove_waiter(waiter_t *w) {
    assert_thread();
    switch (state) {
        case state_unpulsed:
            waiters.remove(w);
            break;
        case state_pulsing:
            /* We want to allow any waiters that have not yet been notified to be removed
            in response to another waiter being notified. */
            waiters.remove(w);
            break;
        case state_pulsed:
            crash("This signal has already been pulsed, and therefore cannot have any "
                "waiters on it.");
        default:
            unreachable();
    }
}

bool signal_t::is_pulsed() const {
    assert_thread();
    switch (state) {
        case state_unpulsed:
            return false;
        case state_pulsing:
            return true;
        case state_pulsed:
            return true;
        default:
            unreachable();
    }
}

signal_t::signal_t() :
    state(state_unpulsed)
    { }

signal_t::~signal_t() {
    switch (state) {
        case state_unpulsed:
            rassert(waiters.empty(), "Destroying a signal_t with something waiting on it");
            break;
        case state_pulsing:
            crash("Destroying a signal_t in response to its own pulse() method is "
                "a bad idea.");
        case state_pulsed:
            rassert(waiters.empty());   // Sanity check
            break;
        default:
            unreachable();
    }
}

void signal_t::pulse() {

    assert_thread();

    rassert(state == state_unpulsed);
    state = state_pulsing;

    /* Notify waiters. We have to be careful how we loop over the waiters because
    any of the waiters that we haven't notified yet could be removed in response
    to notifying one of the waiters before it. See `remove_waiter()` for the rationale
    for allowing this. */
    while (waiter_t *w = waiters.head()) {
        waiters.remove(w);

        /* `on_signal_pulsed()` shouldn't block, because then the other waiters
        wouldn't be notified until it was done, and that's unlikely to be the
        desired behavior. However, it's OK for it to call `notify_now()` or
        `spawn_now()`, which is why we use `ASSERT_FINITE_CORO_WAITING` instead
        of `ASSERT_NO_CORO_WAITING`. */
        ASSERT_FINITE_CORO_WAITING;

        w->on_signal_pulsed();
    }

    state = state_pulsed;
}
