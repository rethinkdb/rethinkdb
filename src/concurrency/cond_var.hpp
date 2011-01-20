#ifndef __CONCURRENCY_COND_VAR_HPP__
#define __CONCURRENCY_COND_VAR_HPP__

#include "arch/arch.hpp"
#include "errors.hpp"

// Let's use C++0x and use std::move!  Let's do it!  NOWWWWW!

// value_type should be something copyable.
template <class value_type>
class unicond_t {
public:
    unicond_t() : waiter_(NULL), satisfied_(false), done_waiting_(false), value_() { }

    void pulse(value_type value) {
        guarantee(!satisfied_);
        value_ = value;
        satisfied_ = true;
        if (waiter_) {
            waiter_->notify();
        }
    }

    value_type wait() {
        guarantee(waiter_ == NULL);
        if (!satisfied_) {
            waiter_ = coro_t::self();
            coro_t::wait();
            guarantee(satisfied_);
        }
        done_waiting_ = true;
        return value_;
    }

    void reset() {
        guarantee(done_waiting_);
        waiter_ = NULL;
        satisfied_ = false;
        done_waiting_ = false;
        value_ = value_type();
    }

private:
    coro_t *waiter_;
    bool satisfied_;
    bool done_waiting_;
    value_type value_;

    DISABLE_COPYING(unicond_t);
};



/* A cond_t is a condition variable suitable for use between coroutines. It is unsafe to use it
except between multiple coroutines on the same thread. It can only be used once. */

struct cond_t {

    cond_t() : ready(false) { }
    void pulse() {
        assert(!ready);
        ready = true;
        for (int i = 0; i < (int)waiters.size(); i++) {
            waiters[i]->notify();
        }
    }
    void wait() {
        if (!ready) {
            waiters.push_back(coro_t::self());
            coro_t::wait();
        }
        assert(ready);
    }

private:
    bool ready;
    std::vector<coro_t *> waiters;

    DISABLE_COPYING(cond_t);
};

/* A value_cond_t is a condition variable combined with a "return value", of sorts, that
is transmitted to the thing waiting on the condition variable.

TODO: Is this actually used in more than one place? */

template<class val_t>
struct value_cond_t {

    value_cond_t() : value(NULL) { }
    void pulse(val_t v) {
        value = new val_t(v);
        cond.pulse();
    }
    val_t wait() {
        cond.wait();
        return *value;
    }
    ~value_cond_t() {
        if (value) delete value;
    }

private:
    cond_t cond;
    val_t *value;
};

#endif /* __CONCURRENCY_COND_VAR_HPP__ */
