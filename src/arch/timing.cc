#include "arch/timing.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "arch/runtime/runtime.hpp"
#include "concurrency/wait_any.hpp"

// nap()

void nap(int ms) THROWS_NOTHING {
    if (ms > 0) {
        signal_timer_t timer(ms);
        timer.wait_lazily_ordered();
    }
}

void nap(int ms, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    signal_timer_t timer(ms);
    wait_interruptible(&timer, interruptor);
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

repeating_timer_t::repeating_timer_t(int frequency_ms, repeating_timer_callback_t *_ringee) :
    ringee(_ringee) {
    rassert(frequency_ms > 0);
    timer = add_timer(frequency_ms, &repeating_timer_t::on_timer_ring, this);
}

repeating_timer_t::~repeating_timer_t() {
    cancel_timer(timer);
}

void call_ringer(repeating_timer_callback_t *ringee) {
    // It would be very easy for a user of repeating_timer_t to have
    // object lifetime errors, if their ring function blocks.  So we
    // have this assertion.
    ASSERT_FINITE_CORO_WAITING;
    ringee->on_ring();
}

void repeating_timer_t::on_timer_ring(void *v_timer) {
    // Spawn _now_, otherwise the reating_timer_t lifetime might end
    // before ring gets used.
    coro_t::spawn_now_dangerously(boost::bind(call_ringer, reinterpret_cast<repeating_timer_t *>(v_timer)->ringee));
}
