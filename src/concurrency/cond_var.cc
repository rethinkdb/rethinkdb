// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "concurrency/cond_var.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/coroutines.hpp"
#include "do_on_thread.hpp"

void cond_t::pulse_if_not_already_pulsed() {
    assert_thread();
    if (!is_pulsed()) {
        pulse();
    }
}

void one_waiter_cond_t::pulse() {
    rassert(!pulsed_);
    pulsed_ = true;
    if (waiter_) {
        coro_t *tmp = waiter_;
        waiter_ = nullptr;
        tmp->notify_later_ordered();
    }
}

void one_waiter_cond_t::wait_ordered() {
    rassert(!waiter_);
    if (!pulsed_) {
        waiter_ = coro_t::self();
        coro_t::wait();
        rassert(pulsed_);
    }
}
