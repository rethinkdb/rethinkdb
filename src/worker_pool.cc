
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
void create_worker(worker_t *worker, worker_fn_t worker_fn) {
    worker->id = -1;
    
    // Create a poll fd
    worker->epoll_fd = epoll_create(0);
    check("Could not create epoll fd", worker->epoll_fd == -1);

    // Start the thread
    int res = pthread_create(&worker->thread, NULL, worker_fn, (void*)worker);
    check("Could not create worker thread", res != 0);
}

/***
 * Pool
 ***/
void create_worker_pool(int workers, worker_pool_t *worker_pool, worker_fn_t worker_fn) {
    worker_pool->nworkers = workers;
    worker_pool->workers = (worker_t*)malloc(sizeof(worker_t) * workers);
    for(int i = 0; i < workers; i++) {
        create_worker(&worker_pool->workers[i], worker_fn);
        worker_pool->workers[i].id = i;
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

