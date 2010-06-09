
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "worker_pool.hpp"
#include "utils.hpp"

using namespace std;

int get_core_info(cmd_config_t *cmd_config) {
    int ncores = get_cpu_count(),
        nmaxcores = cmd_config->max_cores <= 0  ? ncores : cmd_config->max_cores,
        nusecores = min(ncores, nmaxcores);

    // TODO: can we move the printing out of here?
    printf("Physical cores: %d\n", ncores);
    printf("Max cores: ");
    if(cmd_config->max_cores <= 0)
        printf("N/A");
    else
        printf("%d", nmaxcores);
    printf("\n");
    printf("Using cores: %d\n", nusecores);

    return nusecores;
}

void worker_pool_t::create_worker_pool(event_handler_t event_handler, pthread_t main_thread,
                                       int _nworkers)
{
    this->main_thread = main_thread;

    // Create the workers
    nworkers = _nworkers;

    // Init the cache
    cache = new cache_t(BTREE_BLOCK_SIZE, cmd_config->max_cache_size);
    cache->init((char*)cmd_config->db_file_name);
    
    // TODO: there is a good chance here event queue structures may
    // end up sharing a cache line if they're not packed to cache line
    // size. We should just use an array of pointers here.
    workers = (event_queue_t*)malloc(sizeof(event_queue_t) * nworkers);
    for(int i = 0; i < nworkers; i++) {
        new ((void*)&workers[i]) event_queue_t(i, nworkers, event_handler, this);
    }
    active_worker = 0;

    // Now that we set up all event queues, go through them and pass
    // information across hubs
    typedef event_queue_t* event_queue_ptr_t;
    event_queue_ptr_t *queues = new event_queue_ptr_t [nworkers];
    for(int i = 0; i < nworkers; i++) {
        queues[i] = &workers[i];
    }
    for(int i = 0; i < nworkers; i++) {
        workers[i].message_hub.init(i, nworkers, queues);
    }
    delete[] queues;

    // TODO: consider creating lower priority threads to standby in
    // case main threads block.
}

worker_pool_t::worker_pool_t(event_handler_t event_handler, pthread_t main_thread,
                             cmd_config_t *_cmd_config)
    : cache(NULL), cmd_config(_cmd_config)
{

    create_worker_pool(event_handler, main_thread, get_core_info(cmd_config));
}

worker_pool_t::worker_pool_t(event_handler_t event_handler, pthread_t main_thread,
                             int _nworkers, cmd_config_t *_cmd_config)
    : cache(NULL), cmd_config(_cmd_config)
{
    // Currently, get_core_info is called only for printing here. We
    // can get rid of it once we move printing elsewhere.
    get_core_info(cmd_config);
    create_worker_pool(event_handler, main_thread, _nworkers);
}

worker_pool_t::~worker_pool_t() {
    for(int i = 0; i < nworkers; i++) {
        workers[i].~event_queue_t();
    }
    free(workers);
    nworkers = 0;
    cache->close();
    delete cache;
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

