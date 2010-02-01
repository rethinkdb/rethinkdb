
#ifndef __WORKER_POOL_HPP__
#define __WORKER_POOL_HPP__

#include "event_queue.hpp"
#include "arch/resource.hpp"
#include "btree/btree.hpp"

// Worker pool
struct worker_pool_t {
public:
    typedef code_config_t::btree_t btree_t;
    
public:
    worker_pool_t(event_handler_t event_handler, pthread_t main_thread);
    worker_pool_t(event_handler_t event_handler, pthread_t main_thread, int _nworkers);
    ~worker_pool_t();
    
    event_queue_t* next_active_worker();
    
    event_queue_t *workers;
    int nworkers;
    int active_worker;
    pthread_t main_thread;
    btree_t btree;

private:
    void create_worker_pool(event_handler_t event_handler, pthread_t main_thread,
                            int _nworkers);
};

#endif // __WORKER_POOL_HPP__

