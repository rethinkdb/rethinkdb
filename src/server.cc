#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include <signal.h>
#include "config/args.hpp"
#include "utils.hpp"
#include "worker_pool.hpp"
#include "arch/arch.hpp"
#include "server.hpp"

void process_socket(net_conn_t *conn, worker_pool_t *worker_pool) {
    
    // Grab the queue where this socket will go
    event_queue_t *event_queue = worker_pool->next_active_worker()->event_queue;
    
    // TODO: Use the message hub instead of ITC messages.
    
    itc_event_t event;
    bzero(&event, sizeof(itc_event_t));
    event.event_type = iet_new_socket;
    event.data = (void*)conn;
    event_queue->post_itc_message(&event);
}

void start_server(worker_pool_t *worker_pool) {
    
    net_listener_t listener(worker_pool->cmd_config->port);

    printf("Server started\n");

    // Start the server loop
    while(true) {
    
        net_conn_t *conn = listener.accept_blocking();
        
        // Break out of the loop on sigterm
        if (!conn) break;

        process_socket(conn, worker_pool);
    }
}

