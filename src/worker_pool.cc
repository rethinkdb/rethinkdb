
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "worker_pool.hpp"
#include "utils.hpp"

void worker_pool_t::create_worker_pool(event_handler_t event_handler, pthread_t main_thread,
                                       int _nworkers)
{
    main_thread = main_thread;
    nworkers = _nworkers;
    workers = (event_queue_t*)malloc(sizeof(event_queue_t) * nworkers);
    for(int i = 0; i < nworkers; i++) {
        create_event_queue(&workers[i], i, event_handler, this);
    }
    active_worker = 0;
    // TODO: consider creating lower priority threads to standby in
    // case main threads block.
}

worker_pool_t::worker_pool_t(event_handler_t event_handler, pthread_t main_thread)
    : btree(BTREE_BLOCK_SIZE)
{
    int ncpus = get_cpu_count();
    printf("Number of CPUs: %d\n", ncpus);
    create_worker_pool(event_handler, main_thread, ncpus);
}

worker_pool_t::worker_pool_t(event_handler_t event_handler, pthread_t main_thread,
                             int _nworkers)
    : btree(BTREE_BLOCK_SIZE)
{
    create_worker_pool(event_handler, main_thread, _nworkers);
}

worker_pool_t::~worker_pool_t() {
    for(int i = 0; i < nworkers; i++) {
        destroy_event_queue(&workers[i]);
    }
    free(workers);
    nworkers = 0;
}

event_queue_t* next_active_worker(worker_pool_t *worker_pool) {
    int worker = worker_pool->active_worker++;
    if(worker_pool->active_worker >= worker_pool->nworkers)
        worker_pool->active_worker = 0;
    return &worker_pool->workers[worker];
    // TODO: consider introducing randomness to avoid potential
    // (intentional and unintentional) attacks on memory allocation
    // and CPU utilization.
}

