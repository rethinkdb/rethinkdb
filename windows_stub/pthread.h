#pragma once

// TODO ATN

#include "windows.hpp"

struct pthread_attr_t { };
int pthread_attr_init(pthread_attr_t*);
int pthread_attr_setstacksize(pthread_attr_t*, size_t);
int pthread_attr_destroy(pthread_attr_t*);

typedef HANDLE pthread_t;
int pthread_create(pthread_t *, const pthread_attr_t *, void *(*) (void *), void *);
int pthread_join(pthread_t, void**);

typedef struct { } pthread_spinlock_t;

typedef CRITICAL_SECTION pthread_mutex_t;
int pthread_mutex_init(pthread_mutex_t*, void*);
int pthread_mutex_destroy(pthread_mutex_t*);
int pthread_mutex_lock(pthread_mutex_t*);
int pthread_mutex_unlock(pthread_mutex_t*);

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
