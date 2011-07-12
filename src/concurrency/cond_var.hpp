#ifndef __CONCURRENCY_COND_VAR_HPP__
#define __CONCURRENCY_COND_VAR_HPP__

#include "errors.hpp"
#include <boost/bind.hpp>

#include "concurrency/signal.hpp"

/* A cond_t is the simplest form of signal. It just exposes the pulse() method directly.
It is safe to call pulse() on any thread.

If you want something that's like a cond_t but that you can "un-pulse", you should
use resettable_cond_t. */

class coro_t;

struct cond_t : public signal_t {
    cond_t() { }
    void pulse();
private:
    void do_pulse();

    bool ready;
    coro_t *waiter;
    DISABLE_COPYING(cond_t);
};

/* It used to be that cond_t was not thread-safe, and instead there was a "threadsafe_cond_t"
that was thread-safe. Now cond_t is thread-safe, so "threadsafe_cond_t" is obsolete and should
eventually go away. TODO: Get rid of this. */

typedef cond_t threadsafe_cond_t;

/* cond_weak_ptr_t is a pointer to a cond_t, but it NULLs itself when the
cond_t is pulsed. */

struct cond_weak_ptr_t : private signal_t::waiter_t {
    cond_weak_ptr_t() : cond(NULL) { }
    virtual ~cond_weak_ptr_t() {
        rassert(!cond);
    }
    void watch(cond_t *c);

    void pulse_if_non_null() {
        if (cond) {
            cond->pulse();  // Calls on_signal_pulsed()
                            // It's not safe to access local variables at this point; we might have
                            // been deleted.
        }
    }

private:
    cond_t *cond;
    void on_signal_pulsed() {
        rassert(cond);
        cond = NULL;
    }
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
};

/* cond_link_t pulses a given cond_t if a given signal_t is pulsed. */

struct cond_link_t : private signal_t::waiter_t {
    cond_link_t(signal_t *s, cond_t *d) : source(NULL) {
        if (s->is_pulsed()) {
            d->pulse();
        } else {
            source = s;
            dest.watch(d);
            source->add_waiter(this);
        }
    }
    ~cond_link_t() {
        if (source) source->remove_waiter(this);
    }
private:
    signal_t *source;
    void on_signal_pulsed() {
        source = NULL;   // So we don't do an extra remove_waiter() in our destructor
        dest.pulse_if_non_null();
    }
    cond_weak_ptr_t dest;
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
