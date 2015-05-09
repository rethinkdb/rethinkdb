#pragma once

// RSI

typedef struct { } pthread_t;

typedef struct { } pthread_spinlock_t;

typedef struct { } pthread_mutex_t;
int pthread_mutex_init(pthread_mutex_t*, void*);
int pthread_mutex_destroy(pthread_mutex_t*);
int pthread_mutex_lock(pthread_mutex_t*);
int pthread_mutex_unlock(pthread_mutex_t*);

typedef struct { } pthread_cond_t;
int pthread_cond_init(pthread_cond_t*, void*);
int pthread_cond_destroy(pthread_cond_t*);
int pthread_cond_wait(pthread_cond_t*, pthread_mutex_t*);
int pthread_cond_signal(pthread_cond_t*);
int pthread_cond_broadcast(pthread_cond_t*);
