
#ifndef __WORKER_POOL_HPP__
#define __WORKER_POOL_HPP__

#include <pthread.h>
#include <libaio.h>

// Worker
typedef void* (*worker_fn_t)(void*);
struct worker_pool_t;
struct worker_t {
    int id;
    pthread_t aio_thread;
    io_context_t aio_context;
    pthread_t epoll_thread;
    int epoll_fd;
    worker_pool_t *pool;
};

// Worker pool
struct worker_pool_t {
    int file_fd;
    worker_t *workers;
    int nworkers;
    int active_worker;
};

void create_worker_pool(int workers, worker_pool_t *worker_pool,
                        worker_fn_t epoll_fn, worker_fn_t aio_poll_fn);
void destroy_worker_pool(worker_pool_t *worker_pool);
int next_active_worker(worker_pool_t *worker_pool);

#endif // __WORKER_POOL_HPP__

