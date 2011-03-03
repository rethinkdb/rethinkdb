#ifndef __ARCH_LINUX_CONCURRENCY_HPP__
#define __ARCH_LINUX_CONCURRENCY_HPP__

#include <pthread.h>

// Class that wraps a pthread mutex
class system_mutex_t {
    pthread_mutex_t m;
    friend class lock_t;
    friend class system_cond_t;
public:
    system_mutex_t() {
        int res = pthread_mutex_init(&m, NULL);
        guarantee(res == 0, "Could not initialize pthread mutex.");
    }
    ~system_mutex_t() {
        int res = pthread_mutex_destroy(&m);
        guarantee(res == 0, "Could not destroy pthread mutex.");
    }
    class lock_t {
        system_mutex_t *parent;
    public:
        lock_t(system_mutex_t *p) : parent(p) {
            int res = pthread_mutex_lock(&parent->m);
            guarantee(res == 0, "Could not acquire pthread mutex.");
        }
        void unlock() {
            guarantee(parent);
            int res = pthread_mutex_unlock(&parent->m);
            guarantee(res == 0, "Could not release pthread mutex.");
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
        guarantee(res == 0, "Could not initialize pthread cond.");
    }
    ~system_cond_t() {
        int res = pthread_cond_destroy(&c);
        guarantee(res == 0, "Could not destroy pthread cond.");
    }
    void wait(system_mutex_t *mutex) {
        int res = pthread_cond_wait(&c, &mutex->m);
        guarantee(res == 0, "Could not wait on pthread cond.");
    }
    void signal() {
        int res = pthread_cond_signal(&c);
        guarantee(res == 0, "Could not signal pthread cond.");
    }
    void broadcast() {
        int res = pthread_cond_broadcast(&c);
        guarantee(res == 0, "Could not broadcast pthread cond.");
    }
};

#endif /* __ARCH_LINUX_CONCURRENCY_HPP__ */

