#ifndef ARCH_WINDOWS_STUB_PTHREAD_H_
#define ARCH_WINDOWS_STUB_PTHREAD_H_

// An implementation of a subset of pthread for windows

#include "windows.hpp"

struct pthread_attr_t { };
int pthread_attr_init(pthread_attr_t*);
int pthread_attr_setstacksize(pthread_attr_t*, size_t);
int pthread_attr_destroy(pthread_attr_t*);

typedef HANDLE pthread_t;
int pthread_create(pthread_t *, const pthread_attr_t *, void *(*) (void *), void *);
int pthread_join(pthread_t, void**);

typedef CRITICAL_SECTION pthread_spinlock_t;
#define PTHREAD_PROCESS_PRIVATE 0
int pthread_spin_init(pthread_spinlock_t*, int);
int pthread_spin_destroy(pthread_spinlock_t*);
int pthread_spin_lock(pthread_spinlock_t*);
int pthread_spin_unlock(pthread_spinlock_t*);

typedef CRITICAL_SECTION pthread_mutex_t;
int pthread_mutex_init(pthread_mutex_t*, void*);
int pthread_mutex_destroy(pthread_mutex_t*);
int pthread_mutex_lock(pthread_mutex_t*);
int pthread_mutex_unlock(pthread_mutex_t*);

struct pthread_rwlock_t {
    enum class srw_lock_mode_t { SHARED, EXCLUSIVE };
    srw_lock_mode_t current_acq_mode;
    SRWLOCK lock;
};
int pthread_rwlock_init(pthread_rwlock_t*, void*);
int pthread_rwlock_destroy(pthread_rwlock_t*);
int pthread_rwlock_rdlock(pthread_rwlock_t*);
int pthread_rwlock_wrlock(pthread_rwlock_t*);
int pthread_rwlock_unlock(pthread_rwlock_t*);

typedef CONDITION_VARIABLE pthread_cond_t;
int pthread_cond_init(pthread_cond_t*, void*);
int pthread_cond_destroy(pthread_cond_t*);
int pthread_cond_wait(pthread_cond_t*, pthread_mutex_t*);
int pthread_cond_signal(pthread_cond_t*);
int pthread_cond_broadcast(pthread_cond_t*);

#define pthread_once_t thread_local bool
const bool PTHREAD_ONCE_INIT = false;
const bool PTHREAD_ONCE_COMPLETED = true;
int pthread_once(bool *, void(*)(void));

#endif
