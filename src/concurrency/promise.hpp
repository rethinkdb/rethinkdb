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

    const val_t &wait() {
        assert_thread();
        cond.wait_lazily_unordered();
        return *value.get();
    }

    signal_t *get_ready_signal() {
        return &cond;
    }

    const signal_t *get_ready_signal() const {
        return &cond;
    }

    // TODO: get_value is very questionable.
    const val_t &get_value() const {
        assert_thread();
        return *value.get();
    }

    bool is_pulsed() {
        return cond.is_pulsed();
    }

private:
    cond_t cond;
    object_buffer_t<val_t> value;

    DISABLE_COPYING(promise_t);
};

#endif /* CONCURRENCY_PROMISE_HPP_ */
