#ifndef __CONCURRENCY_COND_VAR_HPP__
#define __CONCURRENCY_COND_VAR_HPP__

#include "do_on_thread.hpp"
#include "arch/core.hpp"
#include "concurrency/signal.hpp"

/* A cond_t is the simplest form of signal. It just exposes the pulse() method directly.
It is safe to call pulse() on any thread.

If you want something that's like a cond_t but that you can "un-pulse", you should
use resettable_cond_t. */

struct cond_t : public signal_t {
    cond_t() { }
    void pulse() {
        do_on_thread(home_thread(), boost::bind(&cond_t::do_pulse, this));
    }
private:
    void do_pulse() {
        signal_t::pulse();
    }

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
    void watch(cond_t *c) {
        rassert(!cond);
        rassert(c);
        if (!c->is_pulsed()) {
            cond = c;
            cond->add_waiter(this);
        }
    }

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
struct multi_cond_t {
    multi_cond_t() : ready(false) {}
    void pulse() {
        rassert(!ready);
        ready = true;
        for (waiter_t *w = waiters.head(); w; w = waiters.next(w)) {
            w->coro->notify();
        }
        waiters.clear();
    }

    void wait() {
        if (!ready) {
            waiter_t waiter(coro_t::self());
            waiters.push_back(&waiter);
            coro_t::wait();
        }
        /* It's not safe to assert ready here because the multi_cond_t may have been
        destroyed by now. */
    }

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

    void pulse() {
        rassert(!pulsed_);
        pulsed_ = true;
        if (waiter_) {
            coro_t *tmp = waiter_;
            waiter_ = NULL;
            tmp->notify_now();
            // we might be destroyed here
        }
    }

    void wait_eagerly() {
        rassert(!waiter_);
        if (!pulsed_) {
            waiter_ = coro_t::self();
            coro_t::wait();
            rassert(pulsed_);
        }
    }

private:
    bool pulsed_;
    coro_t *waiter_;

    DISABLE_COPYING(one_waiter_cond_t);
};

#endif /* __CONCURRENCY_COND_VAR_HPP__ */
