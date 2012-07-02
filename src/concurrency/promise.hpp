#ifndef CONCURRENCY_PROMISE_HPP_
#define CONCURRENCY_PROMISE_HPP_

#include "concurrency/cond_var.hpp"

/* A promise_t is a condition variable combined with a "return value", of sorts, that
is transmitted to the thing waiting on the condition variable. */

template <class val_t>
struct promise_t : public home_thread_mixin_t {

    promise_t() : value(NULL) { }
    void pulse(const val_t &v) {
        assert_thread();
        rassert(value == NULL);
        value = new val_t(v);
        cond.pulse();
    }
    const val_t &wait() {
        assert_thread();
        cond.wait_lazily_unordered();
        return *value;
    }
    signal_t *get_ready_signal() {
        return &cond;
    }
    const val_t &get_value() {
        assert_thread();
        rassert(value);
        return *value;
    }
    bool is_pulsed() {
        return cond.is_pulsed();
    }
    ~promise_t() {
        if (value) delete value;
    }

private:
    cond_t cond;
    val_t *value;

    DISABLE_COPYING(promise_t);
};

#endif /* CONCURRENCY_PROMISE_HPP_ */
