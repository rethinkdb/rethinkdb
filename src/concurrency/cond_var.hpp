#ifndef __CONCURRENCY_COND_VAR_HPP__
#define __CONCURRENCY_COND_VAR_HPP__

#include "errors.hpp"
#include "concurrency/signal.hpp"

/* A cond_t is the simplest form of signal. It just exposes the pulse() method directly.
It is safe to call pulse() on any thread.

If you want something that's like a cond_t but that you can "un-pulse", you should
use resettable_cond_t. */

class coro_t;

class cond_t : public signal_t {
public:
    cond_t() { }
    void pulse();
private:
    void do_pulse();

    bool ready;
    coro_t *waiter;
    DISABLE_COPYING(cond_t);
};

/* A multi_cond is a condition variable that can be waited on by multiple
 * things. Pulse will unlock everything that was waiting on it.
 * It is NOT threadsafe. */
class multi_cond_t {
public:
    multi_cond_t() : ready(false) {}
    void pulse();

    void wait();

private:
    bool ready;
    struct waiter_t
         : public intrusive_list_node_t<waiter_t> 
    {
        waiter_t(coro_t *coro) : coro(coro) {}
        coro_t *coro;
    };

    intrusive_list_t<waiter_t> waiters;

    DISABLE_COPYING(multi_cond_t);
};

/* cond_link_t pulses a given cond_t if a given signal_t is pulsed. */

class cond_link_t {
public:
    cond_link_t(signal_t *s, cond_t *d) :
        subs(boost::bind(&cond_link_t::go, this)),
        dest(d)
    {
        if (s->is_pulsed()) go();
        else subs.resubscribe(s);
    }
private:
    void go() {
        if (!dest->is_pulsed()) dest->pulse();
    }
    signal_t::subscription_t subs;
    cond_t *dest;
    DISABLE_COPYING(cond_link_t);
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
