
#ifndef __WORKER_POOL_HPP__
#define __WORKER_POOL_HPP__

#include "event_queue.hpp"

// Worker pool
struct worker_pool_t {
    void *data;
    event_queue_t *workers;
    int nworkers;
    int active_worker;
};

void create_worker_pool(worker_pool_t *worker_pool, event_handler_t event_handler);
void create_worker_pool(worker_pool_t *worker_pool, event_handler_t event_handler, int workers);
void destroy_worker_pool(worker_pool_t *worker_pool);
event_queue_t* next_active_worker(worker_pool_t *worker_pool);

#endif // __WORKER_POOL_HPP__

