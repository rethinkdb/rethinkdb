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
