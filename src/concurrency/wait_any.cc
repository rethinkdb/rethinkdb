// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "concurrency/wait_any.hpp"

void wait_any_t::wait_any_subscription_t::run() {
    parent->pulse_if_not_already_pulsed();
}

wait_any_t::~wait_any_t() {
    while (!subs.empty()) {
        wait_any_subscription_t *p = subs.head();
        subs.remove(p);

        if (p->on_heap()) {
            delete p;
        }
    }
}

void wait_any_t::add(const signal_t *s) {
    rassert(s);
    wait_any_subscription_t *sub;

    // Use preallocated subscriptions, if possible, to save on dynamic memory usage
    if (subs.size() < default_preallocated_subs) {
        sub = sub_storage[subs.size()].create(this, false);
    } else {
        sub = new wait_any_subscription_t(this, true);
    }
    sub->reset(const_cast<signal_t *>(s));
    subs.push_back(sub);
}

void wait_any_t::pulse_if_not_already_pulsed() {
    if (!is_pulsed()) pulse();
}
