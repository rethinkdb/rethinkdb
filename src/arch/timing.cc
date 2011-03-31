#include "arch/timing.hpp"

// nap()

static void wake_coroutine(void *coro) {
    reinterpret_cast<coro_t*>(coro)->notify_now();
}

void nap(int ms) {
    rassert(coro_t::self(), "Not in a coroutine context");
    rassert(ms >= 0);

    if (ms != 0) {
        (void) fire_timer_once(ms, &wake_coroutine, reinterpret_cast<void*>(coro_t::self()));
        coro_t::wait();
    } else {
        coro_t::yield();
    }
}

// pulse_after_time()

struct pulse_waiter_t : public multicond_t::waiter_t {
    pulse_waiter_t(multicond_t *mc) : ttok(NULL), mc(mc) { }
    timer_token_t *ttok;
    multicond_t *mc;
    void on_multicond_pulsed() {
        cancel_timer(ttok);
        delete this;
    }
    static void on_timer(void *pw) {
        pulse_waiter_t *self = static_cast<pulse_waiter_t*>(pw);
        self->mc->remove_waiter(self);
        self->mc->pulse();
        delete self;
    }
};

void pulse_after_time(multicond_t *mc, int ms) {
    pulse_waiter_t *pw = new pulse_waiter_t(mc);
    pw->ttok = fire_timer_once(ms, &pulse_waiter_t::on_timer, static_cast<void*>(pw));
    mc->add_waiter(pw);
}

