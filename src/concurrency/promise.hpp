// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_PROMISE_HPP_
#define CONCURRENCY_PROMISE_HPP_

#include "concurrency/cond_var.hpp"

#include "containers/object_buffer.hpp"

/* A promise_t is a condition variable combined with a "return value", of sorts, that
is transmitted to the thing waiting on the condition variable. */

template <class val_t>
class promise_t : public home_thread_mixin_debug_only_t {
public:
    promise_t() { }

    void pulse(const val_t &v) {
        assert_thread();
        value.create(v);
        cond.pulse();
    }

    void pulse_if_not_already_pulsed(const val_t &v) {
        if (!is_pulsed()) {
            pulse(v);
        }
    }

    const val_t &wait() const {
        assert_thread();
        cond.wait_lazily_unordered();
        return *value.get();
    }

    const signal_t *get_ready_signal() const {
        return &cond;
    }

    /* Note that `assert_get_value()` can be called on any thread. */
    val_t assert_get_value() const {
        cond.guarantee_pulsed();
        return *value.get();
    }

    MUST_USE bool try_get_value(val_t *out) const {
        if (is_pulsed()) {
            *out = *value.get();
            return true;
        } else {
            return false;
        }
    }

    bool is_pulsed() const {
        return cond.is_pulsed();
    }

private:
    cond_t cond;
    object_buffer_t<val_t> value;

    DISABLE_COPYING(promise_t);
};

#endif /* CONCURRENCY_PROMISE_HPP_ */
