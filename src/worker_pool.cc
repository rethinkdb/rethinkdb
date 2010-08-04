
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <strings.h>
#include <string>
#include <sstream>
#include "config/cmd_args.hpp"
#include "config/code.hpp"
#include "utils.hpp"
#include "worker_pool.hpp"
#include "buffer_cache/mirrored.hpp"
#include "buffer_cache/page_map/array.hpp"
#include "buffer_cache/page_repl/page_repl_random.hpp"
#include "buffer_cache/writeback/writeback.hpp"
#include "buffer_cache/concurrency/rwi_conc.hpp"
#include "serializer/in_place.hpp"

worker_t::worker_t(int _workerid, int _nqueues, event_handler_t event_handler,
        worker_pool_t *parent_pool, cmd_config_t *cmd_config) {
    event_queue = gnew<event_queue_t>(_workerid, _nqueues, event_handler, parent_pool, cmd_config);


    // Init the slices
    nworkers = _nqueues;
    workerid = _workerid;
    nslices = cmd_config->n_slices;

    mkdir(DATA_DIRECTORY, 0777);

    for (int i = 0; i < nslices; i++) {
        typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > rdbstring_t;
        typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > rdbstringstream_t;
        rdbstringstream_t out;
        rdbstring_t str((char*)cmd_config->db_file_name);
        out << workerid << "_" << i;
        str += out.str();
        slices[i] = gnew<cache_t>(
                (char*)str.c_str(),
                BTREE_BLOCK_SIZE,
                cmd_config->max_cache_size / event_queue->nqueues,
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
    for (int i = 0; i < nslices; i++)
        slices[i]->start();
}

void worker_t::shutdown_slices() {
    for (int i = 0; i < nslices; i++)
        slices[i]->shutdown(event_queue);
}

void worker_t::delete_slices() {
    for (int i = 0; i < nslices; i++)
        gdelete(slices[i]);
    active_slices = false;
}


void worker_t::on_sync() {
    event_queue->on_sync();
}

void worker_pool_t::create_worker_pool(event_handler_t event_handler, pthread_t main_thread,
                                       int _nworkers, int _nslices)
{
    this->main_thread = main_thread;

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
        workers[i] = gnew<worker_t>(i, nworkers, event_handler, this, cmd_config);
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

    // TODO: decouple number of threads from number of cores and
    // slices.
}

worker_pool_t::worker_pool_t(event_handler_t event_handler, pthread_t main_thread,
                             cmd_config_t *_cmd_config)
    : cmd_config(_cmd_config)
{
    create_worker_pool(event_handler, main_thread, cmd_config->n_workers, cmd_config->n_slices);
}

worker_pool_t::worker_pool_t(event_handler_t event_handler, pthread_t main_thread,
                             int _nworkers, int _nslices, cmd_config_t *_cmd_config)
    : cmd_config(_cmd_config)
{
    // Currently, get_core_info is called only for printing here. We
    // can get rid of it once we move printing elsewhere.
    create_worker_pool(event_handler, main_thread, _nworkers, _nslices);
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

