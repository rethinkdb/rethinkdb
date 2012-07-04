#ifndef CONCURRENCY_PROMISE_HPP_
#define CONCURRENCY_PROMISE_HPP_

#include "concurrency/cond_var.hpp"

#include "containers/scoped.hpp"

/* A promise_t is a condition variable combined with a "return value", of sorts, that
is transmitted to the thing waiting on the condition variable. */

template <class val_t>
struct promise_t : public home_thread_mixin_t {

    promise_t() : value(NULL) { }
    void pulse(const val_t &v) {
        assert_thread();
        value.init(new val_t(v));
        cond.pulse();
    }
    const val_t &wait() {
        assert_thread();
        cond.wait_lazily_unordered();
        return *value.get();
    }
    signal_t *get_ready_signal() {
        return &cond;
    }
    // TODO: get_value is very questionable.
    const val_t &get_value() {
        assert_thread();
        return *value.get();
    }
    bool is_pulsed() {
        return cond.is_pulsed();
    }

private:
    cond_t cond;
    // TODO: Does this need to be heap-allocated?
    scoped_ptr_t<val_t> value;

    DISABLE_COPYING(promise_t);
};

#endif /* CONCURRENCY_PROMISE_HPP_ */
