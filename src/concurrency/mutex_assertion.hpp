#ifndef __CONCURRENCY_MUTEX_ASSERTION_HPP__
#define __CONCURRENCY_MUTEX_ASSERTION_HPP__

#include "utils.hpp"

/* `mutex_assertion_t` is like a mutex, except that it raises an assertion if
there is contention. */

#ifndef NDEBUG

class mutex_assertion_t : public home_thread_mixin_t {
public:
    class acq_t {
    public:
        acq_t() : mutex(NULL) { }
        explicit acq_t(mutex_assertion_t *m) : mutex(NULL) {
            reset(m);
        }
        ~acq_t() {
            reset(NULL);
        }
        void reset(mutex_assertion_t *m = NULL) {
            if (mutex) {
                mutex->assert_thread();
                rassert(mutex->locked);
                mutex->locked = false;
            }
            mutex = m;
            if (mutex) {
                mutex->assert_thread();
                rassert(!mutex->locked);
                mutex->locked = true;
            }
        }
        void assert_is_holding(mutex_assertion_t *m) {
            rassert(mutex == m);
        }
    private:
        friend void swap(acq_t &, acq_t &);
        mutex_assertion_t *mutex;
        DISABLE_COPYING(acq_t);
    };
    explicit mutex_assertion_t(int explicit_home_thread) :
        home_thread_mixin_t(explicit_home_thread), locked(false) { }
    mutex_assertion_t() : locked(false) { }
    ~mutex_assertion_t() {
        rassert(!locked);
    }
private:
    friend class acq_t;
    bool locked;
    DISABLE_COPYING(mutex_assertion_t);
};

inline void swap(mutex_assertion_t::acq_t &a, mutex_assertion_t::acq_t &b) {
    std::swap(a.mutex, b.mutex);
}

#else

class mutex_assertion_t {
public:
    class acq_t {
    public:
        acq_t() { }
        explicit acq_t(mutex_assertion_t *) { }
        void reset(mutex_assertion_t * = NULL) { }
        void assert_is_holding(mutex_assertion_t *) { }
    private:
        DISABLE_COPYING(acq_t);
    };
    explicit mutex_assertion_t(int) { }
    mutex_assertion_t() { }
private:
    DISABLE_COPYING(mutex_assertion_t);
};

inline void swap(mutex_assertion_t::acq_t &, mutex_assertion_t::acq_t &) {
}

#endif /* NDEBUG */

#endif /* __CONCURRENCY_MUTEX_ASSERTION_HPP__ */
