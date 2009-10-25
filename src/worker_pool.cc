
#include <unistd.h>
#include <sys/epoll.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include "worker_pool.hpp"
#include "utils.hpp"

/***
 * Worker
 ***/
void create_worker(worker_pool_t *worker_pool, int id, worker_t *worker,
                   worker_fn_t epoll_fn, worker_fn_t aio_poll_fn) {
    int res;
    worker->id = id;
    worker->pool = worker_pool;

    // Create aio context
    res = io_setup(300, &worker->aio_context);
    check("Could not setup aio context", res != 0);
    
    // Start aio poll thread
    res = pthread_create(&worker->aio_thread, NULL, aio_poll_fn, (void*)worker);
    check("Could not create aio_poll thread", res != 0);
    
    // Create a poll fd
    worker->epoll_fd = epoll_create(0);
    check("Could not create epoll fd", worker->epoll_fd == -1);

    // Start the epoll thread
    res = pthread_create(&worker->epoll_thread, NULL, epoll_fn, (void*)worker);
    check("Could not create epoll thread", res != 0);
}

/***
 * Pool
 ***/
void create_worker_pool(int workers, worker_pool_t *worker_pool,
                        worker_fn_t epoll_fn, worker_fn_t aio_poll_fn) {
    worker_pool->nworkers = workers;
    worker_pool->workers = (worker_t*)malloc(sizeof(worker_t) * workers);
    for(int i = 0; i < workers; i++) {
        create_worker(worker_pool, i, &worker_pool->workers[i], epoll_fn, aio_poll_fn);
    }
    worker_pool->active_worker = 0;
}

void destroy_worker_pool(worker_pool_t *worker_pool) {
    // TODO: ensure threads are terminated
    free(worker_pool->workers);
    worker_pool->nworkers = 0;
}

int next_active_worker(worker_pool_t *worker_pool) {
    int worker = worker_pool->active_worker++;
    if(worker_pool->active_worker >= worker_pool->nworkers)
        worker_pool->active_worker = 0;
    return worker;
}

