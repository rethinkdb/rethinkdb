
#ifndef __WORKER_POOL_HPP__
#define __WORKER_POOL_HPP__

#include <pthread.h>

// Worker
typedef void* (*worker_fn_t)(void*);
struct worker_t {
    pthread_t thread;
    int epoll_fd;
    int id;
};

// Worker pool
struct worker_pool_t {
    worker_t *workers;
    int nworkers;
    int active_worker;
};

void create_worker_pool(int workers, worker_pool_t *worker_pool, worker_fn_t worker_fn);
void destroy_worker_pool(worker_pool_t *worker_pool);
int next_active_worker(worker_pool_t *worker_pool);

#endif // __WORKER_POOL_HPP__

