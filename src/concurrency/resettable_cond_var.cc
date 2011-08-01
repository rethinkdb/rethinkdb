#include "concurrency/resettable_cond_var.hpp"
#include "concurrency/cond_var.hpp"

resettable_cond_t::resettable_cond_t() : state(false), signal(new cond_t) { }

// Put here so that the header file can be minimized.
resettable_cond_t::~resettable_cond_t() { }

void resettable_cond_t::pulse() {
    rassert(state == false);
    state = true;
    signal->pulse();
}
void resettable_cond_t::reset() {
    rassert(state == true);
    state = false;
    signal.reset(new cond_t);
}
signal_t *resettable_cond_t::get_signal() {
    return signal.get();
}
