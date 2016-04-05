// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_CONCURRENCY_HPP_
#define ARCH_IO_CONCURRENCY_HPP_

#include <pthread.h>

#include "errors.hpp"

// Class that wraps a pthread mutex
class system_mutex_t {
    pthread_mutex_t m;
    friend class lock_t;
    friend class system_cond_t;
public:
    system_mutex_t() {
        int res = pthread_mutex_init(&m, nullptr);
        guarantee_xerr(res == 0, res, "Could not initialize pthread mutex.");
    }
    ~system_mutex_t() {
        int res = pthread_mutex_destroy(&m);
        guarantee_xerr(res == 0, res, "Could not destroy pthread mutex.");
    }
    class lock_t {
        system_mutex_t *parent;
    public:
        explicit lock_t(system_mutex_t *p) : parent(p) {
            int res = pthread_mutex_lock(&parent->m);
            guarantee_xerr(res == 0, res, "Could not acquire pthread mutex.");
        }
        void unlock() {
            guarantee(parent);
            int res = pthread_mutex_unlock(&parent->m);
            guarantee_xerr(res == 0, res, "Could not release pthread mutex.");
            parent = nullptr;
        }
        ~lock_t() {
            if (parent) unlock();
        }
    };
};

// Class that wraps a pthread cond
class system_cond_t {
    pthread_cond_t c;
public:
    system_cond_t() {
        int res = pthread_cond_init(&c, nullptr);
        guarantee_xerr(res == 0, res, "Could not initialize pthread cond.");
    }
    ~system_cond_t() {
        int res = pthread_cond_destroy(&c);
        guarantee_xerr(res == 0, res, "Could not destroy pthread cond.");
    }
    void wait(system_mutex_t *mutex) {
        int res = pthread_cond_wait(&c, &mutex->m);
        guarantee_xerr(res == 0, res, "Could not wait on pthread cond.");
    }
    void signal() {
        int res = pthread_cond_signal(&c);
        guarantee_xerr(res == 0, res, "Could not signal pthread cond.");
    }
    void broadcast() {
        int res = pthread_cond_broadcast(&c);
        guarantee_xerr(res == 0, res, "Could not broadcast pthread cond.");
    }
};

// Class that wraps a pthread rwlock
class system_rwlock_t {
    pthread_rwlock_t l;
public:
    system_rwlock_t() {
        int res = pthread_rwlock_init(&l, nullptr);
        guarantee_xerr(res == 0, res, "Could not initialize pthread rwlock.");
    }
    ~system_rwlock_t() {
        int res = pthread_rwlock_destroy(&l);
        guarantee_xerr(res == 0, res, "Could not destroy pthread rwlock.");
    }

    // For low-level use only. If you want to use this for anything else, please
    // define a matching acq_t RAII type.
    void lock_read() {
        int res = pthread_rwlock_rdlock(&l);
        guarantee_xerr(res == 0, res, "Could not acquire pthread rwlock for read.");
    }
    void lock_write() {
        int res = pthread_rwlock_wrlock(&l);
        guarantee_xerr(res == 0, res, "Could not acquire pthread rwlock for write.");
    }
    void unlock() {
        int res = pthread_rwlock_unlock(&l);
        guarantee_xerr(res == 0, res, "Could not release pthread rwlock.");
    }
private:
    DISABLE_COPYING(system_rwlock_t);
};

#endif /* ARCH_IO_CONCURRENCY_HPP_ */

