
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "event_queue.hpp"
#include "worker_pool.hpp"
#include "fsm.hpp"

// Process commands received from the user
void process_command(char *buf, size_t size, event_queue_t *event_queue, event_t *event) {
    int res;
    
    if(size >= 4 && strncmp(buf, "quit", 4) == 0) {
        // Quit the server
        printf("Quitting server...\n");
        res = pthread_kill(event_queue->parent_pool->main_thread, SIGINT);
        check("Could not send kill signal to main thread", res != 0);
    }
    else {
        // TODO: do the whole read from file/send business
        printf("Gotta do smtg here...\n");
    }
}

// This state represents a connected socket with no outstanding
// operations. Incoming events should be user commands received by the
// socket.
void fsm_socket_connected(event_queue_t *event_queue, event_t *event) {
    size_t sz;
    char buf[256];

    // TODO: Are we checking for reading here? What about when socket
    // is ready to write?
    if(event->event_type == et_sock) {
        sz = read(event->state->source, buf, sizeof(buf));
        check("Could not read from socket", sz == -1);
        if(sz > 0) {
            process_command(buf, sz, event_queue, event);
        } else {
            // No data left, close the socket
            printf("Closing socket %d\n", event->state->source);
            queue_forget_resource(event_queue, event->state->source);
            close(event->state->source);
            // TODO: free associated fsm state
            // TODO: if the fsm is not in a finished state, free any
            // intermediate associated resources.
            // TODO: what about keepalive
        }
    } else {
        check("Invalid event", 1);
    }
}

// Switch on the current state and call the appropriate transition
// function.
void do_transition(event_queue_t *event_queue, event_t *event) {
    fsm_state_t *state = (fsm_state_t*)event->state;
    assert(state);
    
    // TODO: Using parent_pool member variable within state
    // transitions might cause cache line alignment issues. Can we
    // eliminate it (perhaps by giving each thread its own private
    // copy of the necessary data)?

    switch(state->state) {
    case fsm_state_t::fsm_socket_connected:
        fsm_socket_connected(event_queue, event);
        break;
    default:
        check("Invalid state", 1);
    }
}

