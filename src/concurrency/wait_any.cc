#include "concurrency/wait_any.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

wait_any_t::wait_any_t() {
}

wait_any_t::wait_any_t(signal_t *s1) {
    add(s1);
}

wait_any_t::wait_any_t(signal_t *s1, signal_t *s2) {
    add(s1);
    add(s2);
}

wait_any_t::wait_any_t(signal_t *s1, signal_t *s2, signal_t *s3) {
    add(s1);
    add(s2);
    add(s3);
}

wait_any_t::wait_any_t(signal_t *s1, signal_t *s2, signal_t *s3, signal_t *s4) {
    add(s1);
    add(s2);
    add(s3);
    add(s4);
}

wait_any_t::wait_any_t(signal_t *s1, signal_t *s2, signal_t *s3, signal_t *s4, signal_t *s5) {
    add(s1);
    add(s2);
    add(s3);
    add(s4);
    add(s5);
}

void wait_any_t::add(signal_t *s) {
    rassert(s);
    if (s->is_pulsed()) {
        pulse_if_not_already_pulsed();
    } else {
        subs.push_back(new signal_t::subscription_t(
            boost::bind(&wait_any_t::pulse_if_not_already_pulsed, this),
            s));
    }
}

void wait_any_t::pulse_if_not_already_pulsed() {
    if (!is_pulsed()) pulse();
}
