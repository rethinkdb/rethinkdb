#ifndef __CONCURRENCY_COND_VAR_HPP__
#define __CONCURRENCY_COND_VAR_HPP__

#include "arch/arch.hpp"
#include "errors.hpp"

// Let's use C++0x and use std::move!  Let's do it!  NOWWWWW!

class interruptable_cond_token_t {
    template <class value_type>
    friend class interruptable_cond_t;
    uint64_t version_number_;
};


template <class value_type>
class interruptable_cond_t {
    typedef interruptable_cond_token_t token_t;
public:

    interruptable_cond_t() : waiter_(NULL), value_(), satisfied_(false), version_(0) { }

    // HACK, probably INCORRECT.  A version of pulse that doesn't take
    // a version.  TODO get rid of this, perhaps.
    void pulse(value_type value) {
        value_ = value;
        satisfied_ = true;
        if (waiter_) {
            waiter_->notify();
        }
    }

    void pulse(token_t version, value_type value) {
        if (version.version_number_ == version_) {
            value_ = value;
        }
    }

    bool wait(token_t version, value_type *out) {
        if (version.version_number_ == version_) {
            if (satisfied_) {
                *out = value_;
                return true;
            } else {
                waiter_ = coro_t::self();
                coro_t::wait();
                if (version.version_number_ == version_) {
                    rassert(satisfied_);
                    *out = value_;
                    return true;
                } else {
                    return false;
                }
            }
        } else {
            return false;
        }
    }

    token_t reset() {
        ++ version_;
        if (waiter_) {
            rassert(!satisfied_);
            waiter_->notify();
        } else {
            satisfied_ = false;
            value_ = value_type();
        }
        token_t ret;
        ret.version_number_ = version_;
        return ret;
    }

private:
    coro_t *waiter_;
    value_type value_;
    bool satisfied_;
    uint64_t version_;

    DISABLE_COPYING(interruptable_cond_t);
};


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

    // TODO: This is criminally insane, neh?  This type eventually
    // calls delete on the address of a parameter that had been passed
    // by reference.

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
