#ifndef __CONCURRENCY_PROMISE_HPP__
#define __CONCURRENCY_PROMISE_HPP__

#include "concurrency/cond_var.hpp"

/* A promise_t is a condition variable combined with a "return value", of sorts, that
is transmitted to the thing waiting on the condition variable. */

template <class val_t>
struct promise_t {

    promise_t() : value(NULL) { }
    void pulse(const val_t &v) {
        value = new val_t(v);
        cond.pulse();
    }
    val_t wait() {
        cond.wait();
        return *value;
    }
    ~promise_t() {
        if (value) delete value;
    }

private:
    cond_t cond;
    val_t *value;

    DISABLE_COPYING(promise_t);
};

// A flat_promise_t is like a promise_t except that it doesn't use a
// pointer, doesn't own the value.
template <class T>
class flat_promise_t {
public:
    flat_promise_t() : cond_(), value_() { }
    ~flat_promise_t() { }
    void pulse(const T& v) {
        value_ = v;
        cond_.pulse();
    }
    T wait() {
        cond_.wait();
        return value_;
    }

private:
    cond_t cond_;
    T value_;

    DISABLE_COPYING(flat_promise_t);
};

#endif /* __CONCURRENCY_PROMISE_HPP__ */
