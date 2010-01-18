
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "event_queue.hpp"
#include "worker_pool.hpp"
#include "fsm.hpp"
#include "utils.hpp"

// Process commands received from the user
int process_command(event_queue_t *event_queue, event_t *event) {
    int res;

    fsm_state_t *state = (fsm_state_t*)event->state;
    char *buf = state->buf;
    unsigned int size = state->nbuf;

    // Make sure the string is properly terminated
    if(buf[size - 1] != '\n' && buf[size - 1] != '\r') {
        return 1;
    }

    // Grab the command out of the string
    unsigned int token_size;
    char delims[] = " \t\n\r\0";
    char *token = tokenize(buf, size, delims, &token_size);
    if(token == NULL)
        return -1;
    
    // Execute command
    if(token_size == 4 && strncmp(token, "quit", 4) == 0) {
        if((token = tokenize(token + token_size,
                             buf + size - (token + token_size),
                             delims, &token_size)) != NULL) {
            return -1;
        }
        // Quit the server
        printf("Quitting server...\n");
        res = pthread_kill(event_queue->parent_pool->main_thread, SIGINT);
        check("Could not send kill signal to main thread", res != 0);
    } else {
        // TODO: unknown command
        char err_msg[] = "(ERROR) Unkown command";
        int len = strlen(err_msg);
        int sz = write(event->state->source, err_msg, len);
        check("Couldn't send message to client", sz != len);
    }
    return 0;
}

// This state represents a connected socket with no outstanding
// operations. Incoming events should be user commands received by the
// socket.
void fsm_socket_ready(event_queue_t *event_queue, event_t *event) {
    int res;
    size_t sz;
    fsm_state_t *state = (fsm_state_t*)event->state;

    // TODO: Are we checking for reading here? What about when socket
    // is ready to write?
    if(event->event_type == et_sock) {
        if(state->buf == NULL) {
            state->buf = (char*)event_queue->alloc.malloc<io_buffer_t>();
            state->nbuf = 0;
        }
        // TODO: we assume the command will fit comfortably into
        // IO_BUFFER_SIZE. We'll need to implement streaming later.
        sz = read(state->source,
                  state->buf + state->nbuf,
                  IO_BUFFER_SIZE - state->nbuf);
        check("Could not read from socket", sz == -1);
        if(sz > 0) {
            state->nbuf += sz;
            res = process_command(event_queue, event);
            if(res == 0 || res == -1) {
                state->state = fsm_state_t::fsm_socket_connected;
                // Command is either completed or malformed, in any
                // cse free the IO buffer
                event_queue->alloc.free((io_buffer_t*)state->buf);
                state->buf = NULL;
                state->nbuf = 0;
            } else if(res == 1) {
                state->state = fsm_state_t::fsm_socket_recv_incomplete;
            }
        } else {
            // No data left, destroy the connection
            queue_forget_resource(event_queue, state->source);
            fsm_destroy_state(state, event_queue);
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
void fsm_do_transition(event_queue_t *event_queue, event_t *event) {
    fsm_state_t *state = (fsm_state_t*)event->state;
    assert(state);
    
    // TODO: Using parent_pool member variable within state
    // transitions might cause cache line alignment issues. Can we
    // eliminate it (perhaps by giving each thread its own private
    // copy of the necessary data)?

    switch(state->state) {
    case fsm_state_t::fsm_socket_connected:
    case fsm_state_t::fsm_socket_recv_incomplete:
        fsm_socket_ready(event_queue, event);
        break;
    default:
        check("Invalid state", 1);
    }
}

void fsm_init_state(fsm_state_t *state) {
    state->state = fsm_state_t::fsm_socket_connected;
    state->buf = NULL;
    state->nbuf = 0;
}

void fsm_destroy_state(fsm_state_t *state, event_queue_t *event_queue) {
    if(state->buf) {
        event_queue->alloc.free((io_buffer_t*)state->buf);
    }
    if(state->source != -1) {
        printf("Closing socket %d\n", state->source);
        close(state->source);
    }
    event_queue->live_fsms.remove(state);
    event_queue->alloc.free(state);
}
