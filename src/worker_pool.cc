#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <strings.h>
#include <string>
#include <sstream>
#include <signal.h>
#include <time.h>
#include <sys/vtimes.h>
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
#include "serializer/log/log_serializer.hpp"
#include "conn_fsm.hpp"
#include "buffer_cache/concurrency/rwi_conc.hpp"

#include "btree/get_fsm.hpp"
#include "btree/get_cas_fsm.hpp"
#include "btree/set_fsm.hpp"
#include "btree/incr_decr_fsm.hpp"
#include "btree/append_prepend_fsm.hpp"
#include "btree/delete_fsm.hpp"

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

    for (int i = 0; i < nslices; i++) {
        incr_ref_count();
    }

    for (int i = 0; i < nslices; i++) {
        if (slices[i]->shutdown(this)) on_store_shutdown();
    }
}

void worker_t::delete_slices() {
    for (int i = 0; i < nslices; i++)
        gdelete(slices[i]);
    active_slices = false;
}

void worker_t::new_fsm(int data, int &resource, void **source) {
    worker_t::conn_fsm_t *fsm =
        new worker_t::conn_fsm_t(data, event_queue);
    live_fsms.push_back(fsm);
    resource = fsm->get_source();
    *source = fsm;
    printf("Opened socket %d\n", resource);
    curr_connections++;
    total_connections++;
}

void worker_t::deregister_fsm(void *fsm, int &resource) {
    worker_t::conn_fsm_t *cfsm = (worker_t::conn_fsm_t *) fsm;
    printf("Closing socket %d\n", cfsm->get_source());
    resource = cfsm->get_source();
    live_fsms.remove(cfsm);
    shutdown_fsms.push_back(cfsm);
    curr_connections--;
    // TODO: there might be outstanding btrees that we're missing (if
    // we're quitting before the the operation completes). We need to
    // free the btree structure in this case (more likely the request
    // and all the btrees associated with it).
}

bool worker_t::deregister_fsm(int &resource) {
    if (live_fsms.empty()) {
        return false;
    } else {
        worker_t::conn_fsm_t *cfsm = live_fsms.head();
        deregister_fsm(cfsm, resource);
        return true;
    }
}

void worker_t::clean_fsms() {
    for (shutdown_fsms_t::iterator it = shutdown_fsms.begin(); it != shutdown_fsms.end(); it++)
        delete *it;
    shutdown_fsms.clear();
}

void worker_t::initiate_conn_fsm_transition(event_t *event) {
    code_config_t::conn_fsm_t *fsm = (code_config_t::conn_fsm_t*)event->state;
    int res = fsm->do_transition(event);
    if(res == worker_t::conn_fsm_t::fsm_transition_ok || res == worker_t::conn_fsm_t::fsm_no_data_in_socket) {
        // Nothing todo
    } else if(res == worker_t::conn_fsm_t::fsm_shutdown_server) {
        int res = pthread_kill(event_queue->parent_pool->main_thread, SIGINT);
        check("Could not send kill signal to main thread", res != 0);
    } else if(res == worker_t::conn_fsm_t::fsm_quit_connection) {
        int source;
        deregister_fsm(fsm, source);
        event_queue->forget_resource(source);
        clean_fsms();
    } else {
        fail("Unhandled fsm transition result");
    }
}

void worker_t::on_btree_completed(code_config_t::btree_fsm_t *btree_fsm) {
    // We received a completed btree that belongs to another
    // core. Send it off and be merry!
    get_cpu_context()->worker->event_queue->message_hub.store_message(btree_fsm->return_cpu, btree_fsm);
}

void worker_t::process_btree_msg(code_config_t::btree_fsm_t *btree_fsm) {
    if(btree_fsm->is_finished()) {
        if (btree_fsm->request) {
            // We received a completed btree that belongs to us
            btree_fsm->request->on_request_part_completed();
        } else { // A btree not associated with any request.
            delete btree_fsm;
        }
    } else {
        // We received an unfinished btree that we need to process
        code_config_t::store_t *store = slice(&btree_fsm->key);
        if (store->run_fsm(btree_fsm, worker_t::on_btree_completed))
            worker_t::on_btree_completed(btree_fsm);
    }
}

void worker_t::process_perfmon_msg(perfmon_msg_t *msg) {    
    worker_t *worker = get_cpu_context()->worker;
    event_queue_t *queue = worker->event_queue;
    int this_cpu = get_cpu_context()->worker->workerid;
    int return_cpu = msg->return_cpu;
        
    switch(msg->state) {
    case perfmon_msg_t::sm_request:
        // Copy our statistics into the perfmon message and send a response
        msg->perfmon = new perfmon_t();
        msg->perfmon->copy_from(worker->perfmon);
        msg->state = perfmon_msg_t::sm_response;
        break;
    case perfmon_msg_t::sm_response:
        msg->request->on_request_part_completed();
        return;
    case perfmon_msg_t::sm_copy_cleanup:
        // Response has been sent to the client, time to delete the
        // copy
        delete msg->perfmon;
        msg->state = perfmon_msg_t::sm_msg_cleanup;
        break;
    case perfmon_msg_t::sm_msg_cleanup:
        // Copy has been deleted, delete the final message and return
        delete msg;
        return;
    }

    msg->return_cpu = this_cpu;
    queue->message_hub.store_message(return_cpu, msg);
}

void worker_t::process_lock_msg(event_t *event, rwi_lock_t::lock_request_t *lr) {
    lr->callback->on_lock_available();
    delete lr;
}

void worker_t::process_log_msg(log_msg_t *msg) {
    if (msg->del) {
        decr_ref_count();
        delete msg;
    } else {
        assert(workerid == LOG_WORKER);
        log_writer.writef("(%s)Q%d:%s:%d:", msg->level_str(), msg->return_cpu, msg->src_file, msg->src_line);
        log_writer.write(msg->str);

        msg->del = true;
        // No need to change return_cpu because the message will be deleted immediately.
        event_queue->message_hub.store_message(msg->return_cpu, msg);
    }
}

void worker_t::process_read_large_value_msg(code_config_t::read_large_value_msg_t *msg) {
    msg->on_arrival();
}

void worker_t::process_write_large_value_msg(code_config_t::write_large_value_msg_t *msg) {
    msg->on_arrival();
}

// Handle events coming from the event queue
void worker_t::event_handler(event_t *event) {
    if (event->event_type == et_sock) {
        // Got some socket action, let the connection fsm know
        initiate_conn_fsm_transition(event);
    } else if(event->event_type == et_cpu_event) {
        cpu_message_t *msg = (cpu_message_t*)event->state;
        // TODO: Possibly use a virtual function here.
        switch(msg->type) {
        case cpu_message_t::mt_btree:
            process_btree_msg((code_config_t::btree_fsm_t*)msg);
            break;
        case cpu_message_t::mt_lock:
            process_lock_msg(event, (rwi_lock_t::lock_request_t*)msg);
            break;
        case cpu_message_t::mt_perfmon:
            process_perfmon_msg((perfmon_msg_t*)msg);
            break;
        case cpu_message_t::mt_log:
            process_log_msg((log_msg_t *) msg);
            break;
        case cpu_message_t::mt_read_large_value:
            process_read_large_value_msg((code_config_t::read_large_value_msg_t *) msg);
            break;
        case cpu_message_t::mt_write_large_value:
            process_write_large_value_msg((code_config_t::write_large_value_msg_t *) msg);
            break;
        }
    } else {
        fail("Unknown event in event_handler");
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
