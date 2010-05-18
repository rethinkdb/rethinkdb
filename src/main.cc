
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
#include "tty.hpp"
#include "server.hpp"
#include "conn_fsm.hpp"
#include "config/cmd_args.hpp"

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

// Handle events coming from the event queue
void event_handler(event_queue_t *event_queue, event_t *event) {
    if(event->event_type == et_timer) {
        // Nothing to do here, move along
    } else if(event->event_type == et_disk) {
        // TODO: right now we assume that block_id_t is the same thing
        // as event->offset. In the future (once we fix the event
        // system), the block_id_t state should really be stored
        // within the event state. Currently, we cast the offset to
        // block_id_t, but it's a big hack, we need to get rid of this
        // later.

        // Grab the btree FSM
        code_config_t::btree_fsm_t *btree_fsm = (code_config_t::btree_fsm_t*)event->state;
        
        // Let the cache know about the disk action
        btree_fsm->cache->aio_complete((code_config_t::btree_fsm_t::block_id_t)event->offset,
                                       event->buf,
                                       event->op == eo_read ? false : true);
        
        // Generate the cache event and forward it to the appropriate btree fsm
        event->event_type = et_cache;
        code_config_t::btree_fsm_t::transition_result_t res = btree_fsm->do_transition(event);
        if(res == code_config_t::btree_fsm_t::transition_complete) {
            // Booooyahh, btree completed, let the socket fsm know
            event->event_type = et_btree_op_complete;
            event->state = btree_fsm->netfsm;
            initiate_conn_fsm_transition(event_queue, event);
        }
    } else if(event->event_type == et_sock) {
        // Got some socket action, let the connection fsm know
        initiate_conn_fsm_transition(event_queue, event);
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
    int res;

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
        int sockfd = start_server(&worker_pool);

        // Feed the terminal into the listening socket
        do_tty_loop(sockfd);

        // At this point we broke out of the tty loop. Stop the server.
        stop_server(sockfd);
    }

    printf("Server offline\n");
}

