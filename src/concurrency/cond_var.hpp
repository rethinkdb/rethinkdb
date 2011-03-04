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

/* A multi_cond is a condition variable that can be waited on by multiple
 * things. Pulse will unlock everything that was waiting on it.
 * It is NOT threadsafe. */
struct multi_cond_t {
    multi_cond_t() : ready(false) {}
    void pulse() {
        rassert(!ready);
        ready = true;
        for (waiter_list_t::iterator it = waiters.begin(); it != waiters.end(); it++)
            (*it)->coro->notify();
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

    typedef intrusive_list_t<waiter_t> waiter_list_t;
     waiter_list_t waiters;
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

template<class val_t, class underlying_cond_t = cond_t>
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
};

#endif /* __CONCURRENCY_COND_VAR_HPP__ */
