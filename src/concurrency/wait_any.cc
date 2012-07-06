#include "concurrency/wait_any.hpp"

class wait_any_subscription_t : public signal_t::subscription_t, public intrusive_list_node_t<wait_any_subscription_t> {
public:
    explicit wait_any_subscription_t(wait_any_t *parent) : parent_(parent) { }
    virtual void run();
private:
    wait_any_t *parent_;
    DISABLE_COPYING(wait_any_subscription_t);
};

void wait_any_subscription_t::run() {
    parent_->pulse_if_not_already_pulsed();
}


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

wait_any_t::~wait_any_t() {
    while (!subs.empty()) {
        wait_any_subscription_t *p = subs.head();
        subs.remove(p);
        delete p;
    }
}

void wait_any_t::add(signal_t *s) {
    rassert(s);
    wait_any_subscription_t *sub = new wait_any_subscription_t(this);
    sub->reset(s);
    subs.push_back(sub);
}

void wait_any_t::pulse_if_not_already_pulsed() {
    if (!is_pulsed()) pulse();
}

void wait_interruptible(signal_t *signal, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    wait_any_t waiter(signal, interruptor);
    waiter.wait_lazily_unordered();
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}
