// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_MUTEX_ASSERTION_HPP_
#define CONCURRENCY_MUTEX_ASSERTION_HPP_

#include <utility>

#include "threading.hpp"

/* `mutex_assertion_t` is like a mutex, except that it raises an assertion if
there is contention. */

#ifndef NDEBUG

struct mutex_assertion_t : public home_thread_mixin_t {
    struct acq_t {
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

    explicit mutex_assertion_t(threadnum_t explicit_home_thread) :
        home_thread_mixin_t(explicit_home_thread), locked(false) {
    }

    mutex_assertion_t() : locked(false) {
    }

    mutex_assertion_t(mutex_assertion_t &&movee) : locked(movee.locked) {
        rassert(!locked);
    }

    // Unimplemented because nobody uses this yet.
    void operator=(mutex_assertion_t &&movee) = delete;

    ~mutex_assertion_t() { reset(); }
    void reset() { rassert(!locked); }

    void rethread(threadnum_t new_thread) {
        rassert(!locked);
        real_home_thread = new_thread;
    }
private:
    friend struct acq_t;
    bool locked;
    DISABLE_COPYING(mutex_assertion_t);
};

struct rwi_lock_assertion_t : public home_thread_mixin_t {
    struct read_acq_t {
        read_acq_t() : lock(NULL) { }
        explicit read_acq_t(rwi_lock_assertion_t *l) : lock(NULL) {
            reset(l);
        }
        ~read_acq_t() {
            reset(NULL);
        }
        void reset(rwi_lock_assertion_t *l = NULL) {
            if (lock) {
                lock->assert_thread();
                rassert(lock->state > 0);
                lock->state--;
            }
            lock = l;
            if (lock) {
                lock->assert_thread();
                rassert(lock->state != rwi_lock_assertion_t::write_locked);
                lock->state++;
            }
        }
        void assert_is_holding(rwi_lock_assertion_t *l) {
            rassert(lock == l);
        }
    private:
        rwi_lock_assertion_t *lock;
        DISABLE_COPYING(read_acq_t);
    };
    struct write_acq_t {
        write_acq_t() : lock(NULL) { }
        explicit write_acq_t(rwi_lock_assertion_t *l) : lock(NULL) {
            reset(l);
        }
        ~write_acq_t() {
            reset(NULL);
        }
        void reset(rwi_lock_assertion_t *l = NULL) {
            if (lock) {
                lock->assert_thread();
                rassert(lock->state == rwi_lock_assertion_t::write_locked);
                lock->state = 0;
            }
            lock = l;
            if (lock) {
                lock->assert_thread();
                rassert(lock->state == 0);
                lock->state = rwi_lock_assertion_t::write_locked;
            }
        }
        void assert_is_holding(rwi_lock_assertion_t *l) {
            rassert(lock == l);
        }
    private:
        rwi_lock_assertion_t *lock;
        DISABLE_COPYING(write_acq_t);
    };
    explicit rwi_lock_assertion_t(threadnum_t explicit_home_thread) :
        home_thread_mixin_t(explicit_home_thread), state(0) { }
    rwi_lock_assertion_t() : state(0) { }
    ~rwi_lock_assertion_t() {
        rassert(state == 0);
    }
    void rethread(threadnum_t new_thread) {
        rassert(state == 0);
        real_home_thread = new_thread;
    }
private:
    friend struct read_acq_t;
    friend struct write_acq_t;
    static const int write_locked = -1;
    /* If unlocked, `state` will be 0. If read-locked, `state` will be the
    number of readers. If write-locked, `state` will be `write_locked`. */
    int state;
    DISABLE_COPYING(rwi_lock_assertion_t);
};

struct semaphore_assertion_t : public home_thread_mixin_debug_only_t {
    struct acq_t {
        acq_t() : parent(NULL) { }
        explicit acq_t(semaphore_assertion_t *p) : parent(p) {
            parent->assert_thread();
            rassert(parent->capacity > 0);
            parent->capacity--;
        }
        void reset() {
            parent->assert_thread();
            parent->capacity++;
            parent = NULL;
        }
        ~acq_t() {
            if (parent) {
                reset();
            }
        }
    private:
        semaphore_assertion_t *parent;
        DISABLE_COPYING(acq_t);
    };
    explicit semaphore_assertion_t(int cap) : capacity(cap) { }
private:
    int capacity;
    DISABLE_COPYING(semaphore_assertion_t);
};

#else /* NDEBUG */

struct mutex_assertion_t {
    struct acq_t {
        acq_t() { }
        explicit acq_t(mutex_assertion_t *) { }
        /* This is necessary to avoid compiler complaints about unused variables in
        release mode. */
        ~acq_t() { (void)this; }
        void reset(mutex_assertion_t * = nullptr) { }
        void assert_is_holding(mutex_assertion_t *) { }
    private:
        DISABLE_COPYING(acq_t);
    };
    explicit mutex_assertion_t(int) { }
    mutex_assertion_t() { }
    mutex_assertion_t(mutex_assertion_t &&) { }
    void reset() { }
    void rethread(threadnum_t) { }
private:
    DISABLE_COPYING(mutex_assertion_t);
};

inline void swap(mutex_assertion_t::acq_t &, mutex_assertion_t::acq_t &) {
}

struct rwi_lock_assertion_t {
    struct read_acq_t {
        read_acq_t() { }
        explicit read_acq_t(rwi_lock_assertion_t *) { }
        ~read_acq_t() { (void)this; }
        void reset(rwi_lock_assertion_t * = nullptr) { }
        void assert_is_holding(rwi_lock_assertion_t *) { }
    private:
        DISABLE_COPYING(read_acq_t);
    };
    struct write_acq_t {
        write_acq_t() { }
        explicit write_acq_t(rwi_lock_assertion_t *) { }
        ~write_acq_t() { (void)this; }
        void reset(rwi_lock_assertion_t * = nullptr) { }
        void assert_is_holding(rwi_lock_assertion_t *) { }
    private:
        DISABLE_COPYING(write_acq_t);
    };
    explicit rwi_lock_assertion_t(int) { }
    rwi_lock_assertion_t() { }
    void rethread(threadnum_t) { }
private:
    friend struct read_acq_t;
    friend struct write_acq_t;
    DISABLE_COPYING(rwi_lock_assertion_t);
};

struct semaphore_assertion_t {
    struct acq_t {
        acq_t() { }
        explicit acq_t(semaphore_assertion_t *) { }
        void reset() { }
        ~acq_t() { (void)this; }
    private:
        DISABLE_COPYING(acq_t);
    };
    explicit semaphore_assertion_t(int) { }
private:
    DISABLE_COPYING(semaphore_assertion_t);
};

#endif /* NDEBUG */

#endif /* CONCURRENCY_MUTEX_ASSERTION_HPP_ */
