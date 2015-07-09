// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_CONCURRENCY_HPP_
#define ARCH_IO_CONCURRENCY_HPP_

#include <pthread.h>

// Class that wraps a pthread mutex
class system_mutex_t {
    pthread_mutex_t m;
    friend class system_cond_t;
public:
    system_mutex_t() {
        int res = pthread_mutex_init(&m, NULL);
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
            parent = NULL;
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
        int res = pthread_cond_init(&c, NULL);
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

#endif /* ARCH_IO_CONCURRENCY_HPP_ */

