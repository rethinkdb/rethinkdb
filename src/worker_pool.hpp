
#ifndef __WORKER_POOL_HPP__
#define __WORKER_POOL_HPP__

#include <vector>
#include "event_queue.hpp"
#include "arch/resource.hpp"
#include "config/cmd_args.hpp"
#include "config/code.hpp"
#include "corefwd.hpp"
#include "buffer_cache/callbacks.hpp"

typedef void (*event_handler_t)(event_queue_t*, event_t*);

class worker_t : public sync_callback<code_config_t> {
    public:
        typedef code_config_t::cache_t cache_t;
    public:
        worker_t(int queue_id, int _nqueues, event_handler_t event_handler,
                  worker_pool_t *parent_pool, cmd_config_t *cmd_config);
        ~worker_t();
        void start_worker();
        void start_caches();
        void shutdown_caches();
        int slice(btree_key *key);
    public:
        event_queue_t *event_queue;

        int nworkers;

        int nslices;
        cache_t *slices[MAX_SLICES];
    public:
        virtual void on_sync();
};

// Worker pool
struct worker_pool_t {
public:    
    worker_pool_t(event_handler_t event_handler, pthread_t main_thread,
                  cmd_config_t *_cmd_config);
    worker_pool_t(event_handler_t event_handler, pthread_t main_thread, int _nworkers, int _nslices,
                  cmd_config_t *_cmd_config);
    ~worker_pool_t();
    
    worker_t* next_active_worker();
    
    worker_t *workers[MAX_CPUS];
    int nworkers;
    int nslices;
    int active_worker;
    pthread_t main_thread;
    cmd_config_t *cmd_config;

    // Collects thread local allocators for delete after shutdown
    std::vector<void*, gnew_alloc<void*> > all_allocs;
    
private:
    void create_worker_pool(event_handler_t event_handler, pthread_t main_thread,
                            int _nworkers, int _nslices);
};

#endif // __WORKER_POOL_HPP__
