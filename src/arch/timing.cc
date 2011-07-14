#include "arch/timing.hpp"

#include "arch/arch.hpp"
#include "arch/coroutines.hpp"


// nap()

void nap(int ms) {
    if (ms > 0) {
        signal_timer_t timer(ms);
        timer.wait_lazily();
    } else {
        coro_t::yield();
    }
}

// signal_timer_t

signal_timer_t::signal_timer_t(int ms) {
    rassert(ms >= 0);
    timer = fire_timer_once(ms, &signal_timer_t::on_timer_ring, this);
}

signal_timer_t::~signal_timer_t() {
    if (timer) cancel_timer(timer);
}

void signal_timer_t::on_timer_ring(void *v_timer) {
    signal_timer_t *self = reinterpret_cast<signal_timer_t*>(v_timer);
    self->timer = NULL;
    self->pulse();
}

// repeating_timer_t

repeating_timer_t::repeating_timer_t(int frequency_ms, boost::function<void(void)> ring) :
    ring(ring) {
    rassert(frequency_ms > 0);
    timer = add_timer(frequency_ms, &repeating_timer_t::on_timer_ring, this);
}

repeating_timer_t::~repeating_timer_t() {
    cancel_timer(timer);
}

void repeating_timer_t::on_timer_ring(void *v_timer) {
    coro_t::spawn(reinterpret_cast<repeating_timer_t*>(v_timer)->ring);
}
