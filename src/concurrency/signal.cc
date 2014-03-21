// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "concurrency/signal.hpp"

#include "arch/runtime/coroutines.hpp"

class notify_later_ordered_subscription_t : public signal_t::subscription_t {
public:
    notify_later_ordered_subscription_t() : coro_(coro_t::self()) { }
    virtual void run() {
        coro_->notify_later_ordered();
    }
private:
    coro_t *coro_;
    DISABLE_COPYING(notify_later_ordered_subscription_t);
};

void signal_t::wait_lazily_ordered() const {
    if (!is_pulsed()) {
        notify_later_ordered_subscription_t subs;
        subs.reset(const_cast<signal_t *>(this));
        coro_t::wait();
    }
}

class notify_sometime_subscription_t : public signal_t::subscription_t {
public:
    notify_sometime_subscription_t() : coro_(coro_t::self()) { }
    virtual void run() {
        coro_->notify_sometime();
    }

private:
    coro_t *coro_;
    DISABLE_COPYING(notify_sometime_subscription_t);
};

void signal_t::wait_lazily_unordered() const {
    if (!is_pulsed()) {
        notify_sometime_subscription_t subs;
        subs.reset(const_cast<signal_t *>(this));
        coro_t::wait();
    }
}


signal_t::signal_t(signal_t &&movee)
    : home_thread_mixin_t(std::move(movee)),
      pulsed(movee.pulsed),
      publisher_controller(std::move(movee.publisher_controller)),
      lock(std::move(movee.lock)) {
    movee.pulsed = false;
}

// The same thing that happens when a signal_t is destructed happens: We crash if
// there are any current waiters.
void signal_t::reset() {
    lock.reset();
    publisher_controller.reset();
    pulsed = false;
}
