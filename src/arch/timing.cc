#include "arch/timing.hpp"
#include "arch/arch.hpp"

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

// call_with_delay()

struct delayed_caller_t : public signal_t::waiter_t {

    delayed_caller_t(int delay_ms, const boost::function<void()> &cb, signal_t *ab) :
        ttok(NULL), callback(cb), aborter(ab)
    {
        rassert(callback);
        if (aborter && aborter->is_pulsed()) {
            delete this;
        } else {
            if (aborter) aborter->add_waiter(this);
            ttok = fire_timer_once(delay_ms, &delayed_caller_t::on_timer, static_cast<void*>(this));
        }
    }

    timer_token_t *ttok;
    boost::function<void()> callback;
    signal_t *aborter;

    void on_signal_pulsed() {
        cancel_timer(ttok);
        delete this;
    }

    static void on_timer(void *s) {
        delayed_caller_t *self = static_cast<delayed_caller_t*>(s);
        if (self->aborter) self->aborter->remove_waiter(self);
        self->callback();
        delete self;
    }
};

void call_with_delay(int delay_ms, const boost::function<void()> &fun, signal_t *aborter) {
    new delayed_caller_t(delay_ms, fun, aborter);
}

// repeating_timer_t

repeating_timer_t::repeating_timer_t(int frequency_ms, boost::function<void(void)> ring) :
    ring(ring) {
    timer = add_timer(frequency_ms, &repeating_timer_t::on_timer_ring, this);
}

repeating_timer_t::~repeating_timer_t() {
    cancel_timer(timer);
}

void repeating_timer_t::on_timer_ring(void *v_timer) {
    coro_t::spawn(reinterpret_cast<repeating_timer_t*>(v_timer)->ring);
}
