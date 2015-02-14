// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_COND_VAR_HPP_
#define CONCURRENCY_COND_VAR_HPP_

#include "errors.hpp"
#include "concurrency/signal.hpp"

/* A cond_t is the simplest form of signal. It just exposes the pulse() method
directly. */

class coro_t;

class cond_t : public signal_t {
public:
    cond_t() { }
    cond_t(cond_t &&movee) : signal_t(std::move(movee)) { }
    void pulse_if_not_already_pulsed();

    using signal_t::pulse;
    using signal_t::reset;

private:
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

    void wait_ordered();

private:
    bool pulsed_;
    coro_t *waiter_;

    DISABLE_COPYING(one_waiter_cond_t);
};

#endif /* CONCURRENCY_COND_VAR_HPP_ */
