#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <strings.h>
#include <string>
#include <sstream>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>   // For mkdir()
#include <sys/vtimes.h>
#include "config/cmd_args.hpp"
#include "config/alloc.hpp"
#include "utils.hpp"
#include "worker_pool.hpp"
#include "conn_fsm.hpp"

//perfmon functions TODO figure out where these should live
int uptime(void) {
    return (int) difftime(time(NULL), get_cpu_context()->worker->parent_pool->starttime);
}

int pointer_size(void) {
    return sizeof (void *);
}

int rusage_user(void) {
    struct vtimes times;
    vtimes(&times, NULL);
    return times.vm_utime;
}

int rusage_system(void) {
    struct vtimes times;
    vtimes(&times, NULL);
    return times.vm_stime;
}

worker_t::worker_t(int _workerid, int _nqueues,
        worker_pool_t *parent_pool, cmd_config_t *cmd_config) {
    ref_count = 0;
    event_queue = gnew<event_queue_t>(_workerid, _nqueues, parent_pool, this, cmd_config);

    total_connections = 0;
    curr_connections = 0;
    curr_items = 0;
    total_items = 0;
    cmd_get = 0;
    cmd_set = 0;
    get_misses = 0;
    get_hits = 0;
    bytes_read = 0;
    bytes_written = 0;

    this->parent_pool = parent_pool;
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "total_connections", (void*)&total_connections, var_monitor_t::var_monitor_t::var_monitor_combine_sum));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "curr_connections", (void*)&curr_connections, var_monitor_t::var_monitor_combine_sum));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "total_items", (void*)&total_items, var_monitor_t::var_monitor_t::var_monitor_combine_sum));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "curr_items", (void*)&curr_items, var_monitor_t::var_monitor_combine_sum));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "pid", (void*)&(parent_pool->pid), var_monitor_t::var_monitor_combine_pass));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_long_int, "start_time", (void*)&(parent_pool->starttime), var_monitor_t::var_monitor_combine_pass));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "threads", (void*)&(parent_pool->nworkers), var_monitor_t::var_monitor_combine_pass));
    /* perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "slices", (void*)&(parent_pool->nslices))); //not part of memcached protocol, uncomment upon risk of death */
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int_function, "uptime", (void*) uptime, var_monitor_t::var_monitor_combine_pass));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int_function, "pointer_size", (void*) pointer_size, var_monitor_t::var_monitor_combine_pass));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int_function, "rusage_user", (void*) rusage_user, var_monitor_t::var_monitor_combine_pass));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int_function, "rusage_system", (void*) rusage_system, var_monitor_t::var_monitor_combine_pass));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "cmd_get", (void*) &cmd_get, var_monitor_t::var_monitor_combine_sum));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "cmd_set", (void*) &cmd_set, var_monitor_t::var_monitor_combine_sum));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "get_misses", (void*) &get_misses, var_monitor_t::var_monitor_combine_sum));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "get_hits", (void*) &get_hits, var_monitor_t::var_monitor_combine_sum));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "bytes_read", (void*) &bytes_read, var_monitor_t::var_monitor_combine_sum));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "bytes_written", (void*) &bytes_written, var_monitor_t::var_monitor_combine_sum));

    cas_counter = 0;

    // Init the slices
    nworkers = _nqueues;
    workerid = _workerid;
    nslices = cmd_config->n_slices;
    
    if (strncmp(cmd_config->db_file_name, DATA_DIRECTORY, strlen(DATA_DIRECTORY)) == 0) {
        mkdir(DATA_DIRECTORY, 0777);
    }

    for (int i = 0; i < nslices; i++) {
        char name[MAX_DB_FILE_NAME];
        int len = snprintf(name, MAX_DB_FILE_NAME, "%s_%d_%d", cmd_config->db_file_name, workerid, i);
        //TODO the below line is currently the only way to write to a block device,
        //we need a command line way to do it, this also requires consoladating to one file
        //int len = snprintf(name, MAX_DB_FILE_NAME, "/dev/sdb");
        check("Name too long", len == MAX_DB_FILE_NAME);
        slices[i] = gnew<store_t>(
                name,
                BTREE_BLOCK_SIZE,
                cmd_config->max_cache_size / nworkers,
                cmd_config->wait_for_flush,
                cmd_config->flush_timer_ms,
                cmd_config->flush_threshold_percent);
    }
    active_slices  = true;
}

worker_t::~worker_t() {
    check("Error in ~worker_t, cannot delete a worker_t without first deleting its slices", active_slices == true);
    delete event_queue;
}

void worker_t::start_worker() {
    event_queue->start_queue(this);
}

void worker_t::start_slices() {
    for (int i = 0; i < nslices; i++) {
        // The key-value store may not start up immediately, but that's okay; it will queue up
        // requests and then run them when it is done starting up.
        slices[i]->start(NULL);
    }
}

void worker_t::shutdown() {
    shutting_down = true;
    
    /* TODO: This will crash if there are outstanding requests that point at the conn_fsm_t */
    while (conn_fsm_t *conn = live_fsms.head()) {
        delete conn;   // Destructor takes care of everything
    }
    
    for (int i = 0; i < nslices; i++) {
        incr_ref_count();
    }

    for (int i = 0; i < nslices; i++) {
        if (slices[i]->shutdown(this)) {
            on_store_shutdown();
        }
    }
}

void worker_t::delete_slices() {
    for (int i = 0; i < nslices; i++)
        gdelete(slices[i]);
    active_slices = false;
}

void worker_t::new_fsm(itc_event_t event) {
    
    net_conn_t *conn = (net_conn_t*)event.data;   // TODO: This is a horrible hack
    
    new conn_fsm_t(conn);
}


void worker_t::on_btree_completed(btree_fsm_t *btree_fsm) {
    // We received a completed btree that belongs to another
    // core. Send it off and be merry!
    // TODO: Move this to the btree_fsm itself.
    if (continue_on_cpu(btree_fsm->return_cpu, btree_fsm)) {
        call_later_on_this_cpu(btree_fsm);
    }
}

void worker_t::incr_ref_count() {
    ref_count++;
}

void worker_t::decr_ref_count() {
    ref_count--;
    if (ref_count == 0 && shutting_down) {
        event_queue->send_shutdown();
    }
}

void worker_t::on_store_shutdown() {
    decr_ref_count();
}

btree_value::cas_t worker_t::gen_cas() {
    // A CAS value is made up of both a timestamp and a per-worker counter,
    // which should be enough to guarantee that it'll be unique.
    return (time(NULL) << 32) | (++cas_counter);
}

void worker_pool_t::create_worker_pool(pthread_t main_thread,
                                       int _nworkers, int _nslices)
{
    this->main_thread = main_thread;
    this->pid = getpid();
    this->starttime = time(NULL);

    // Create the workers
    if (_nworkers != 0)
        nworkers =  std::min(_nworkers, MAX_CPUS);
    else
        nworkers = get_cpu_count();

    if (_nslices != 0)
        nslices = std::min(_nslices, MAX_SLICES);
    else
        nslices = DEFAULT_SLICES;

    cmd_config->n_workers = nworkers;
    cmd_config->n_slices = nslices;

    for(int i = 0; i < nworkers; i++) {
        workers[i] = gnew<worker_t>(i, nworkers, this, cmd_config);
    }
    active_worker = 0;
    
    for(int i = 0; i < nworkers; i++) {
        workers[i]->event_queue->message_hub.init(i, nworkers, workers);
    }
    
    // Start the actual queue
    for(int i = 0; i < nworkers; i++) {
        workers[i]->start_worker();        
    }

    // TODO: can we move the printing out of here?
    printf("Physical cores: %d\n", get_cpu_count());
    printf("Using cores: %d\n", nworkers);
    printf("Slices per core: %d\n", nslices);
    printf("Total RAM: %ldMB\n", get_total_ram() / 1024 / 1024);
    printf("Free RAM: %ldMB (%.2f%%)\n",
           get_available_ram() / 1024 / 1024,
           (double)get_available_ram() / (double)get_total_ram() * 100.0f);
    printf("Max cache size: %ldMB\n",
           cmd_config->max_cache_size / 1024 / 1024);

    // TODO: this whole queue creation business is a mess, refactor
    // the way it works.
}

worker_pool_t::worker_pool_t(pthread_t main_thread,
                             cmd_config_t *_cmd_config)
    : cmd_config(_cmd_config)
{
    create_worker_pool(main_thread, cmd_config->n_workers, cmd_config->n_slices);
}

worker_pool_t::worker_pool_t(pthread_t main_thread,
                             int _nworkers, int _nslices, cmd_config_t *_cmd_config)
    : cmd_config(_cmd_config)
{
    create_worker_pool(main_thread, _nworkers, _nslices);
}

worker_pool_t::~worker_pool_t() {

    // Start stopping each event queue
    for(int i = 0; i < nworkers; i++) {
        workers[i]->event_queue->begin_stopping_queue();
    }
    
    // Wait for all of the event queues to finish stopping before destroying any of them
    for (int i = 0; i < nworkers; i++) {
        workers[i]->event_queue->finish_stopping_queue();
    }
    
    // Free the event queues
    for(int i = 0; i < nworkers; i++) {
        gdelete(workers[i]);
    }
    nworkers = 0;

    // Delete all the custom allocators in the system
    for(size_t i = 0; i < all_allocs.size(); i++) {
        tls_small_obj_alloc_accessor<alloc_t>::alloc_vector_t *allocs
            = (tls_small_obj_alloc_accessor<alloc_t>::alloc_vector_t*)(all_allocs[i]);
        if(allocs) {
            for(size_t j = 0; j < allocs->size(); j++) {
                gdelete(allocs->operator[](j));
            }
        }
        delete allocs;
    }
}


worker_t* worker_pool_t::next_active_worker() {
    int worker = active_worker++;
    if(active_worker >= nworkers)
        active_worker = 0;
    return workers[worker];
    // TODO: consider introducing randomness to avoid potential
    // (intentional and unintentional) attacks on memory allocation
    // and CPU utilization.
}
