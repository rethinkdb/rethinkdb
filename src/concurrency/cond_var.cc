#include "concurrency/cond_var.hpp"

#include "arch/coroutines.hpp"
#include "do_on_thread.hpp"

void cond_t::pulse() {
    do_on_thread(home_thread(), boost::bind(&cond_t::do_pulse, this));
}

void cond_t::do_pulse() {
    signal_t::pulse();
}

void cond_weak_ptr_t::watch(cond_t *c) {
    rassert(!cond);
    rassert(c);
    if (!c->is_pulsed()) {
        cond = c;
        cond->add_waiter(this);
    }
}

void multi_cond_t::pulse() {
    rassert(!ready);
    ready = true;
    for (waiter_t *w = waiters.head(); w; w = waiters.next(w)) {
        w->coro->notify();
    }
    waiters.clear();
}

void multi_cond_t::wait() {
    if (!ready) {
        waiter_t waiter(coro_t::self());
        waiters.push_back(&waiter);
        coro_t::wait();
    }
    /* It's not safe to assert ready here because the multi_cond_t may have been
       destroyed by now. */
}

void one_waiter_cond_t::pulse() {
    rassert(!pulsed_);
    pulsed_ = true;
    if (waiter_) {
        coro_t *tmp = waiter_;
        waiter_ = NULL;
        tmp->notify_now();
        // we might be destroyed here
    }
}

void one_waiter_cond_t::wait_eagerly() {
    rassert(!waiter_);
    if (!pulsed_) {
        waiter_ = coro_t::self();
        coro_t::wait();
        rassert(pulsed_);
    }
}
