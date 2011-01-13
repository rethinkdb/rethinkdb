#ifndef __CONCURRENCY_COND_VAR_HPP__
#define __CONCURRENCY_COND_VAR_HPP__

#include "arch/arch.hpp"

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
};

/* A value_cond_t is a condition variable combined with a "return value", of sorts, that
is transmitted to the thing waiting on the condition variable.

TODO: Is this actually used in more than one place? */

template<class val_t>
struct value_cond_t {

    value_cond_t() { }
    void pulse(val_t v) {
        value = v;
        cond.pulse();
    }
    val_t wait() {
        cond.wait();
        return value;
    }

private:
    cond_t cond;
    val_t value;
};

#endif /* __CONCURRENCY_COND_VAR_HPP__ */
