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
    }
}

void signal_t::remove_waiter(waiter_t *w) {
    assert_thread();
    /* It's safe to call remove_waiter() from within pulse(), because notifying one waiter
    might reasonably cause another waiter to be removed */
    rassert(state != state_pulsed);
    waiters.remove(w);
}

bool signal_t::is_pulsed() {
    assert_thread();
    switch (state) {
        case state_unpulsed:
            return false;
        case state_pulsing:
            /* It's not clear what behavior makes sense in this case. The rationale for returning
            `true` is that someone might call `wait()` again from within the on-pulse callback, and
            we don't want to make them wait in that case. However, it might make more sense to
            crash() in this case. */
            return true;
        case state_pulsed:
            return true;
        default:
            unreachable();
    }
}

signal_t::signal_t() :
    state(state_unpulsed),
    set_true_on_death(NULL)
    { }

signal_t::~signal_t() {
    rassert(waiters.empty());
    if (set_true_on_death) {
        rassert(!*set_true_on_death);
        *set_true_on_death = true;
    }
}

void signal_t::pulse() {

    assert_thread();

    rassert(state == state_unpulsed);
    state = state_pulsing;

    bool we_died = false;
    rassert(!set_true_on_death);
    set_true_on_death = &we_died;

    /* Duplicate `waiters` in case we are deleted */
    intrusive_list_t<waiter_t> waiters_2;
    waiters_2.append_and_clear(&waiters);

    /* Notify waiters */
    while (waiter_t *w = waiters_2.head()) {
        waiters_2.remove(w);
        w->on_signal_pulsed();
    }

    if (!we_died) {
        set_true_on_death = NULL;
        state = state_pulsed;
    }
}
