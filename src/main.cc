
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include "config/cmd_args.hpp"
#include "config/code.hpp"
#include "utils.hpp"
#include "logger.hpp"
#include "btree/node.hpp"
#include "worker_pool.hpp"
#include "server.hpp"
#include "alloc/memalign.hpp"
#include "alloc/pool.hpp"
#include "alloc/dynamic_pool.hpp"
#include "alloc/stats.hpp"
#include "alloc/alloc_mixin.hpp"
#include "config/args.hpp"
#include "arch/io.hpp"
#include "serializer/in_place.hpp"
#include "buffer_cache/mirrored.hpp"
#include "buffer_cache/page_map/array.hpp"
#include "buffer_cache/page_repl/page_repl_random.hpp"
#include "buffer_cache/writeback/writeback.hpp"
#include "buffer_cache/concurrency/rwi_conc.hpp"
#include "btree/node.tcc"
#include "btree/internal_node.tcc"
#include "btree/leaf_node.tcc"
#include "btree/get_fsm.hpp"
#include "btree/set_fsm.hpp"
#include "btree/incr_decr_fsm.hpp"
#include "btree/delete_fsm.hpp"
#include "conn_fsm.hpp"
#include "request.hpp"

// TODO: we should redo the plumbing for the entire callback system so
// that nothing is hardcoded here. Messages should flow dynamically to
// their destinations without having a central place with all the
// handlers.

void initiate_conn_fsm_transition(event_queue_t *event_queue, event_t *event) {
    code_config_t::conn_fsm_t *fsm = (code_config_t::conn_fsm_t*)event->state;
    int res = fsm->do_transition(event);
    if(res == event_queue_t::conn_fsm_t::fsm_transition_ok || res == event_queue_t::conn_fsm_t::fsm_no_data_in_socket) {
        // Nothing todo
    } else if(res == event_queue_t::conn_fsm_t::fsm_shutdown_server) {
        int res = pthread_kill(event_queue->parent_pool->main_thread, SIGINT);
        check("Could not send kill signal to main thread", res != 0);
    } else if(res == event_queue_t::conn_fsm_t::fsm_quit_connection) {
        event_queue->deregister_fsm(fsm);
        delete fsm;
    } else {
        check("Unhandled fsm transition result", 1);
    }
}

void on_btree_completed(code_config_t::btree_fsm_t *btree_fsm) {
    // We received a completed btree that belongs to another
    // core. Send it off and be merry!
    get_cpu_context()->worker->event_queue->message_hub.store_message(btree_fsm->return_cpu, btree_fsm);
}

void process_btree_msg(code_config_t::btree_fsm_t *btree_fsm) {
    worker_t *worker = get_cpu_context()->worker;
    if(btree_fsm->is_finished()) {
        
        // We received a completed btree that belongs to us
        btree_fsm->request->on_request_part_completed();
    
    } else {
        // We received a new btree that we need to process
        
        // The btree is constructed with no cache; here we must assign it its proper cache
        assert(!btree_fsm->cache);
        btree_fsm->cache = worker->slices[worker->slice(&btree_fsm->key)];
        
        btree_fsm->on_complete = on_btree_completed;
        code_config_t::btree_fsm_t::transition_result_t btree_res = btree_fsm->do_transition(NULL);
        if(btree_res == code_config_t::btree_fsm_t::transition_complete) {
            on_btree_completed(btree_fsm);
        }
    }
}

void process_perfmon_msg(perfmon_msg_t *msg)
{    
    event_queue_t *queue = get_cpu_context()->worker->event_queue;
    int this_cpu = queue->queue_id;
    int return_cpu = msg->return_cpu;
        
    switch(msg->state) {
    case perfmon_msg_t::sm_request:
        // Copy our statistics into the perfmon message and send a response
        msg->perfmon = new perfmon_t();
        msg->perfmon->copy_from(queue->perfmon);
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

// TODO: this should really be moved into the event queue.
void process_lock_msg(event_queue_t *event_queue, event_t *event, rwi_lock_t::lock_request_t *lr) {
    lr->callback->on_lock_available();
    delete lr;
}

void process_log_msg(log_msg_t *msg) {
    event_queue_t *queue = get_cpu_context()->worker->event_queue;
    if (msg->del) {
        delete msg;
    } else {
        assert(queue->queue_id == LOG_WORKER);
        queue->log_writer.writef("(%s)Q%d:%s:%d:", msg->level_str(), msg->return_cpu, msg->src_file, msg->src_line);
        queue->log_writer.write(msg->str);

        msg->del = true;
        // No need to change return_cpu because the message will be deleted immediately.
        queue->message_hub.store_message(msg->return_cpu, msg);
    }
}

// Handle events coming from the event queue
void event_handler(event_queue_t *event_queue, event_t *event) {
    if(event->event_type == et_sock) {
        // Got some socket action, let the connection fsm know
        initiate_conn_fsm_transition(event_queue, event);
    } else if(event->event_type == et_cpu_event) {
        cpu_message_t *msg = (cpu_message_t*)event->state;
        switch(msg->type) {
        case cpu_message_t::mt_btree:
            process_btree_msg((code_config_t::btree_fsm_t*)msg);
            break;
        case cpu_message_t::mt_lock:
            process_lock_msg(event_queue, event, (rwi_lock_t::lock_request_t*)msg);
            break;
        case cpu_message_t::mt_perfmon:
            process_perfmon_msg((perfmon_msg_t*)msg);
            break;
        case cpu_message_t::mt_log:
            process_log_msg((log_msg_t *) msg);
            break;
        }
    } else {
        check("Unknown event in event_handler", 1);
    }
}

void term_handler(int signum) {
    // We'll naturally break out of the main loop because the accept
    // syscall will get interrupted.
}

void install_handlers() {
    int res;
    
    // Setup termination handlers
    struct sigaction action;
    bzero((char*)&action, sizeof(action));
    action.sa_handler = term_handler;
    res = sigaction(SIGTERM, &action, NULL);
    check("Could not install TERM handler", res < 0);

    bzero((char*)&action, sizeof(action));
    action.sa_handler = term_handler;
    res = sigaction(SIGINT, &action, NULL);
    check("Could not install INT handler", res < 0);
};

int main(int argc, char *argv[])
{
    // Parse command line arguments
    cmd_config_t config;
    parse_cmd_args(argc, argv, &config);

    // Setup signal handlers
    install_handlers();

    // We're using the scope here to make sure worker_pool is
    // auto-destroyed before the rest of the operations.
    {
        // Create a pool of workers
        worker_pool_t worker_pool(event_handler, pthread_self(), &config);

        // Start the logger
        worker_pool.workers[LOG_WORKER]->event_queue->log_writer.start(&config);

        // Start the server (in a separate thread)
        start_server(&worker_pool);

        // If we got out of start_server, we're about to shut down.
        printf("Shutting down server...\n");
    }
}

