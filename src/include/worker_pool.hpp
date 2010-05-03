
#ifndef __WORKER_POOL_HPP__
#define __WORKER_POOL_HPP__

#include "event_queue.hpp"
#include "arch/resource.hpp"
#include "config/cmd_args.hpp"

// Worker pool
struct worker_pool_t {
public:
    typedef code_config_t::cache_t cache_t;
    
public:
    worker_pool_t(event_handler_t event_handler, pthread_t main_thread,
                  cmd_config_t *_cmd_config);
    worker_pool_t(event_handler_t event_handler, pthread_t main_thread, int _nworkers,
                  cmd_config_t *_cmd_config);
    ~worker_pool_t();
    
    event_queue_t* next_active_worker();
    
    event_queue_t *workers;
    int nworkers;
    int active_worker;
    pthread_t main_thread;
    cache_t *cache;
    cmd_config_t *cmd_config;
    
private:
    void create_worker_pool(event_handler_t event_handler, pthread_t main_thread,
                            int _nworkers);
};

#endif // __WORKER_POOL_HPP__

