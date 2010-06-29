
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

#include "utils.hpp"
#include "worker_pool.hpp"
#include "async_io.hpp"
#include "server.hpp"
#include "config/cmd_args.hpp"
#include "config/code.hpp"
#include "alloc/memalign.hpp"
#include "alloc/pool.hpp"
#include "alloc/dynamic_pool.hpp"
#include "alloc/stats.hpp"
#include "alloc/alloc_mixin.hpp"
#include "config/args.hpp"
#include "arch/io.hpp"
#include "conn_fsm.hpp"
#include "serializer/in_place.hpp"
#include "buffer_cache/fallthrough.hpp"
#include "buffer_cache/stats.hpp"
#include "buffer_cache/mirrored.hpp"
#include "buffer_cache/page_map/unlocked_hash_map.hpp"
#include "buffer_cache/page_repl/none.hpp"
#include "buffer_cache/writeback/writeback.hpp"
#include "buffer_cache/concurrency/rwi_conc.hpp"
#include "btree/get_fsm.hpp"
#include "btree/set_fsm.hpp"
#include "btree/delete_fsm.hpp"
#include "btree/array_node.hpp"
#include "request.hpp"

// TODO: we should redo the plumbing for the entire callback system so
// that nothing is hardcoded here. Messages should flow dynamically to
// their destinations without having a central place with all the
// handlers.

void initiate_conn_fsm_transition(event_queue_t *event_queue, event_t *event) {
    code_config_t::conn_fsm_t *fsm = (code_config_t::conn_fsm_t*)event->state;
    int res = fsm->do_transition(event);
    if(res == event_queue_t::conn_fsm_t::fsm_transition_ok) {
        // Nothing todo
    } else if(res == event_queue_t::conn_fsm_t::fsm_shutdown_server) {
        printf("Shutting down server...\n");
        int res = pthread_kill(event_queue->parent_pool->main_thread, SIGINT);
        check("Could not send kill signal to main thread", res != 0);
    } else if(res == event_queue_t::conn_fsm_t::fsm_quit_connection) {
        event_queue->deregister_fsm(fsm);
    } else {
        check("Unhandled fsm transition result", 1);
    }
}

void on_btree_completed(code_config_t::btree_fsm_t *btree_fsm) {
    // We received a completed btree that belongs to another
    // core. Send it off and be merry!
    get_cpu_context()->event_queue->message_hub.store_message(btree_fsm->return_cpu, btree_fsm);
}

void process_btree_msg(code_config_t::btree_fsm_t *btree_fsm) {
    event_queue_t *event_queue = get_cpu_context()->event_queue;
    // The FSM should work on the local cache, not on the cache of
    // the sender.
    // TODO: btree_fsm should not be constructed with a cache, as
    // it acts both as the cross-cpu message and the fsm.
    btree_fsm->cache = event_queue->cache;
    if(btree_fsm->is_finished()) {
        // We received a completed btree that belongs to us
        btree_fsm->request->ncompleted++;
        if(btree_fsm->request->ncompleted == btree_fsm->request->nstarted) {
            // This should be before build_response, as the
            // request handler will destroy the btree
            code_config_t::conn_fsm_t *netfsm = btree_fsm->request->netfsm;
            event_t event;
            bzero((void*)&event, sizeof(event));
            event.state = netfsm;
            event.event_type = et_request_complete;
            netfsm->req_handler->build_response(btree_fsm->request);
            initiate_conn_fsm_transition(event_queue, &event);
        }
    } else {
        // We received a new btree that we need to process
        btree_fsm->on_complete = on_btree_completed;
        code_config_t::btree_fsm_t::transition_result_t btree_res = btree_fsm->do_transition(NULL);
        if(btree_res == code_config_t::btree_fsm_t::transition_complete) {
            on_btree_completed(btree_fsm);
        }
    }
}

void process_lock_msg(event_queue_t *event_queue, event_t *event, rwi_lock<code_config_t>::lock_request_t *lr) {
    // TODO: currently the cache calls the lock with a buf as user
    // state, and if the object can't be locked right away, we get a
    // message from the lock through the event queue here. We then
    // call notify_callbacks on buf, but really, the cache should be
    // getting this event and calling notify_callbacks.
    code_config_t::buf_t *buf = (code_config_t::buf_t*)lr->state;
    buf->notify_on_lock();
    delete lr;
}

// Handle events coming from the event queue
void event_handler(event_queue_t *event_queue, event_t *event) {
    if(event->event_type == et_timer) {
        // Nothing to do here, move along
    } else if(event->event_type == et_disk) {
        // Let the cache know about the disk action
        event_queue->cache->aio_complete((code_config_t::buf_t*)event->state, event->buf, event->op != eo_read);
    } else if(event->event_type == et_sock) {
        // Got some socket action, let the connection fsm know
        initiate_conn_fsm_transition(event_queue, event);
    } else if(event->event_type == et_cpu_event) {
        cpu_message_t *msg = (cpu_message_t*)event->state;
        switch(msg->type) {
        case cpu_message_t::mt_btree:
            process_btree_msg((code_config_t::btree_fsm_t*)msg);
            break;
        case cpu_message_t::mt_lock:
            process_lock_msg(event_queue, event, (rwi_lock<code_config_t>::lock_request_t*)msg);
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

        // Start the server (in a separate thread)
        start_server(&worker_pool);
    }
}

