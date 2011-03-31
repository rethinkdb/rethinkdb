#ifndef __CONCURRENCY_COND_VAR_HPP__
#define __CONCURRENCY_COND_VAR_HPP__

#include "arch/arch.hpp"
#include "errors.hpp"

// Let's use C++0x and use std::move!  Let's do it!  NOWWWWW!

// value_type should be something copyable.
template <class value_type>
class unicond_t {
public:
    unicond_t() : waiter_(NULL), satisfied_(false), done_waiting_(false), value_() { }

    void pulse(value_type value) {
        guarantee(!satisfied_);
        value_ = value;
        satisfied_ = true;
        if (waiter_) {
            waiter_->notify();
        }
    }

    value_type wait() {
        guarantee(waiter_ == NULL);
        if (!satisfied_) {
            waiter_ = coro_t::self();
            coro_t::wait();
            guarantee(satisfied_);
        }
        done_waiting_ = true;
        return value_;
    }

    void reset() {
        guarantee(done_waiting_);
        waiter_ = NULL;
        satisfied_ = false;
        done_waiting_ = false;
        value_ = value_type();
    }

private:
    coro_t *waiter_;
    bool satisfied_;
    bool done_waiting_;
    value_type value_;

    DISABLE_COPYING(unicond_t);
};



/* A cond_t is a condition variable suitable for use between coroutines. It is unsafe to use it
except between multiple coroutines on the same thread. It can only be used once. */

struct cond_t {
    cond_t() : ready(false), waiter(NULL) { }
    void pulse() {
        rassert(!ready);
        ready = true;
        if (waiter) waiter->notify();
    }
    void wait() {
        if (!ready) {
            waiter = coro_t::self();
            coro_t::wait();
        }
        rassert(ready);
    }

private:
    bool ready;
    coro_t *waiter;

    DISABLE_COPYING(cond_t);
};

/* A multicond_t is a condition variable that more than one thing can wait for. */

struct multicond_t {
    multicond_t() : ready(false) { }

    struct waiter_t : public intrusive_list_node_t<waiter_t> {
        virtual void on_multicond_pulsed() = 0;
    };

    /* pulse() and wait() act just like they do in cond_t */
    void pulse() {
        rassert(!ready);
        ready = true;
        while (waiter_t *w = waiters.head()) {
            waiters.remove(w);
            w->on_multicond_pulsed();
        }
    }
    void wait() {
        if (!ready) {
            struct coro_waiter_t : public waiter_t {
                coro_t *to_wake;
                void on_multicond_pulsed() { to_wake->notify(); }
            } waiter;
            waiter.to_wake = coro_t::self();
            add_waiter(&waiter);
            coro_t::wait();
        }
    }

    /* One major use case for a multicond_t is to wait for one of several different
    events, where all but the event that actually happened need to be cancelled. For example,
    you might wait for some event, but also have a timeout. If the event happens before the
    timeout happens, then the timeout timer must be cancelled. waiter_t is an alternative
    to wait() that is designed to be used by things that will signal a multicond_t, but also
    need to be notified if something else signals the multicond first. */
    void add_waiter(waiter_t *w) {
        if (ready) {
            w->on_multicond_pulsed();
        } else {
            waiters.push_back(w);
        }
    }
    void remove_waiter(waiter_t *w) {
        rassert(!ready);
        waiters.remove(w);
    }

private:
    bool ready;
    intrusive_list_t<waiter_t> waiters;
};

/* A threadsafe_cond_t is a thread-safe condition variable. It's like cond_t except it can be
used with multiple coroutines on different threads. */

struct threadsafe_cond_t {
    threadsafe_cond_t() : ready(false), waiter(NULL) { }
    void pulse() {
        lock.lock();
        rassert(!ready);
        ready = true;
        if (waiter) waiter->notify();
        lock.unlock();
    }
    void wait() {
        lock.lock();
        if (!ready) {
            waiter = coro_t::self();
            lock.unlock();
            coro_t::wait();
        } else {
            lock.unlock();
        }
        rassert(ready);
    }

private:
    bool ready;
    coro_t *waiter;
    spinlock_t lock;

    DISABLE_COPYING(threadsafe_cond_t);
};

/* A promise_t is a condition variable combined with a "return value", of sorts, that
is transmitted to the thing waiting on the condition variable. It's parameterized on an underlying
cond_t type so that you can make it thread-safe if you need to. */

template <class val_t, class underlying_cond_t = cond_t>
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
    underlying_cond_t cond;
    val_t *value;

    DISABLE_COPYING(promise_t);
};

// A flat_promise_t is like a promise_t except that it doesn't use a
// pointer, doesn't own the value.
template <class T, class underlying_cond_t = cond_t>
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
    underlying_cond_t cond_;
    T value_;

    DISABLE_COPYING(flat_promise_t);
};

#endif /* __CONCURRENCY_COND_VAR_HPP__ */
