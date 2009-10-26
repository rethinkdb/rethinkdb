
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "worker_pool.hpp"
#include "utils.hpp"

void create_worker_pool(worker_pool_t *worker_pool, event_handler_t event_handler) {
    int ncpus = get_cpu_count();
    printf("Number of CPUs: %d\n", ncpus);
    create_worker_pool(worker_pool, event_handler, ncpus);
}

void create_worker_pool(worker_pool_t *worker_pool, event_handler_t event_handler, int workers) {
    worker_pool->nworkers = workers;
    worker_pool->workers = (event_queue_t*)malloc(sizeof(event_queue_t) * workers);
    for(int i = 0; i < workers; i++) {
        create_event_queue(&worker_pool->workers[i], i, event_handler);
    }
    worker_pool->active_worker = 0;
    // TODO: consider creating lower priority threads to standby in
    // case main threads block.
}

void destroy_worker_pool(worker_pool_t *worker_pool) {
    for(int i = 0; i < worker_pool->nworkers; i++) {
        destroy_event_queue(&worker_pool->workers[i]);
    }
    free(worker_pool->workers);
    worker_pool->nworkers = 0;
}

event_queue_t* next_active_worker(worker_pool_t *worker_pool) {
    int worker = worker_pool->active_worker++;
    if(worker_pool->active_worker >= worker_pool->nworkers)
        worker_pool->active_worker = 0;
    return &worker_pool->workers[worker];
}

