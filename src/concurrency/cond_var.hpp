#ifndef __CONCURRENCY_COND_VAR_HPP__
#define __CONCURRENCY_COND_VAR_HPP__

#include "errors.hpp"
#include "concurrency/signal.hpp"

/* A cond_t is the simplest form of signal. It just exposes the pulse() method
directly. It is safe to call pulse() on any thread. */

class coro_t;

class cond_t : public signal_t {
public:
    cond_t() { }
    void pulse();
private:
    void do_pulse();

    DISABLE_COPYING(cond_t);
};

class one_waiter_cond_t {
public:
    one_waiter_cond_t() : pulsed_(false), waiter_(NULL) { }
    ~one_waiter_cond_t() {
        rassert(pulsed_);
        rassert(!waiter_);
    }

    void pulse();

    void wait_eagerly();

private:
    bool pulsed_;
    coro_t *waiter_;

    DISABLE_COPYING(one_waiter_cond_t);
};

#endif /* __CONCURRENCY_COND_VAR_HPP__ */
