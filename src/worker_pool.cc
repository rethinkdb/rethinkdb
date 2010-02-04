
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "worker_pool.hpp"
#include "utils.hpp"

void worker_pool_t::create_worker_pool(event_handler_t event_handler, pthread_t main_thread,
                                       int _nworkers)
{
    this->main_thread = main_thread;

    // Create the workers
    nworkers = _nworkers;
    workers = (event_queue_t*)malloc(sizeof(event_queue_t) * nworkers);
    for(int i = 0; i < nworkers; i++) {
        new ((void*)&workers[i]) event_queue_t(i, event_handler, this);
    }
    active_worker = 0;

    // Init the cache
    // TODO: file name clearly shouldn't be hardcoded
    cache.init((char*)"data.file");
    
    // TODO: consider creating lower priority threads to standby in
    // case main threads block.
}

worker_pool_t::worker_pool_t(event_handler_t event_handler, pthread_t main_thread)
    : cache(BTREE_BLOCK_SIZE)
{
    int ncpus = get_cpu_count();
    printf("Number of CPUs: %d\n", ncpus);
    create_worker_pool(event_handler, main_thread, ncpus);
}

worker_pool_t::worker_pool_t(event_handler_t event_handler, pthread_t main_thread,
                             int _nworkers)
    : cache(BTREE_BLOCK_SIZE)
{
    create_worker_pool(event_handler, main_thread, _nworkers);
}

worker_pool_t::~worker_pool_t() {
    for(int i = 0; i < nworkers; i++) {
        workers[i].~event_queue_t();
    }
    free(workers);
    nworkers = 0;
    cache.close();
}

event_queue_t* worker_pool_t::next_active_worker() {
    int worker = active_worker++;
    if(active_worker >= nworkers)
        active_worker = 0;
    return &workers[worker];
    // TODO: consider introducing randomness to avoid potential
    // (intentional and unintentional) attacks on memory allocation
    // and CPU utilization.
}

