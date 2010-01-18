
#ifndef __WORKER_POOL_HPP__
#define __WORKER_POOL_HPP__

#include "event_queue.hpp"
#include "btree/btree.hpp"
#include "btree/array_node.hpp"
#include "buffer_cache/volatile.hpp"
#include "config.hpp"

// The tree we're using
// TODO: This is *VERY* not thread safe
typedef volatile_cache_t rethink_cache_t;
typedef btree<array_node_t<rethink_cache_t::block_id_t>, rethink_cache_t> rethink_tree_t;

// Worker pool
struct worker_pool_t {
    worker_pool_t() : btree(BTREE_BLOCK_SIZE) {}
    event_queue_t *workers;
    int nworkers;
    int active_worker;
    pthread_t main_thread;
    rethink_tree_t btree;
};

void create_worker_pool(worker_pool_t *worker_pool, event_handler_t event_handler, pthread_t main_thread);
void create_worker_pool(worker_pool_t *worker_pool, event_handler_t event_handler, pthread_t main_thread,
                        int workers);
void destroy_worker_pool(worker_pool_t *worker_pool);
event_queue_t* next_active_worker(worker_pool_t *worker_pool);

#endif // __WORKER_POOL_HPP__

