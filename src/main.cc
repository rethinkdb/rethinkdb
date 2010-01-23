
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
#include "fsm.hpp"

void event_handler(event_queue_t *event_queue, event_t *event) {
    if(event->event_type != et_timer) {
        fsm_state_t *state = (fsm_state_t*)event->state;
        if(state->do_transition(event) == 1) {
            printf("Shutting down server...\n");
            int res = pthread_kill(event_queue->parent_pool->main_thread, SIGINT);
            check("Could not send kill signal to main thread", res != 0);
        }
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

    // Setup signal handlers
    install_handlers();

    // We're using the scope here to make sure worker_pool is
    // auto-destroyed before the rest of the operations.
    {
        // Create a pool of workers
        worker_pool_t worker_pool(event_handler, pthread_self());

        // Start the server (in a separate thread)
        int sockfd = start_server(&worker_pool);

        // Feed the terminal into the listening socket
        do_tty_loop(sockfd);

        // At this point we broke out of the tty loop. Stop the server.
        stop_server(sockfd);
    }

    printf("Server offline\n");
}

