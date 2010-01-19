
#include <assert.h>
#include <unistd.h>
#include "event_queue.hpp"
#include "fsm.hpp"
#include "worker_pool.hpp"
#include "networking.hpp"

// Process commands received from the user
int process_command(event_queue_t *event_queue, event_t *event);

// This function returns the socket to clean connected state
void return_to_fsm_socket_connected(event_queue_t *event_queue, fsm_state_t *state) {
    state->state = fsm_state_t::fsm_socket_connected;
    event_queue->alloc.free((io_buffer_t*)state->buf);
    state->buf = NULL;
    state->nbuf = 0;
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
        if(event->op == eo_rdwr || event->op == eo_read) {
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
                if(res == -1) {
                    // Command wasn't processed correctly, send error
                    send_err_to_client(event_queue, state);
                }
                if(res == 0 || res == -1) {
                    // TODO: presumably, since we're using edge
                    // trigerred version of epoll, we should try to
                    // read from the socket again in this case, and
                    // only move into fsm_socket_send_incomplete state
                    // when we can't parse the message *and* read
                    // returns EAGAIN or EWOULDBLOCK. Actually, even
                    // if we parse the message successfully, we should
                    // keep reading until these two error messages
                    // (although no transition to incomplete state is
                    // necessary here). This should be done before
                    // production, least we experience weird behavior.
                    if(state->state != fsm_state_t::fsm_socket_send_incomplete) {
                        // Command is either completed or malformed, in any
                        // case get back to clean connected state
                        return_to_fsm_socket_connected(event_queue, state);
                    } else {
                        // We've sent a partial packet to the client, so
                        // we must remain in the partial send state. At
                        // this point there is nothing to do but wait
                        // until we can write to the socket.
                    }
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
            // The kernel tells us we're ready to write even when we
            // didn't ask it.
        }
    } else {
        check("fsm_socket_ready: Invalid event", 1);
    }
}

// The socket is ready for sending more information and we were in the
// middle of an incomplete send request.
void fsm_socket_send_incomplete(event_queue_t *event_queue, event_t *event) {
    // TODO: incomplete send needs to be tested therally. It's not
    // clear how to get the kernel to artifically limit the send
    // buffer.
    if(event->event_type == et_sock) {
        if(event->op == eo_rdwr || event->op == eo_write) {
            fsm_state_t *state = (fsm_state_t*)event->state;
            send_msg_to_client(event_queue, state);
            if(state->state != fsm_state_t::fsm_socket_send_incomplete) {
                // We've successfully finished sending the message
                return_to_fsm_socket_connected(event_queue, state);
            } else {
                // We still didn't send the full message yet
            }
        } else {
            // TODO: we've received a message from the client while
            // we're in the process of sending him a message. For now
            // we're dropping the message received out of bound, but
            // what is the proper solution here? (note: same is true
            // above if op == eo_rdwr)
            printf("received out-of-bound msg from client (during a partial response)\n");
        }
    } else {
        check("fsm_socket_send_ready: Invalid event", 1);
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
    case fsm_state_t::fsm_socket_send_incomplete:
        fsm_socket_send_incomplete(event_queue, event);
        break;
    default:
        check("Invalid state", 1);
    }
}

void fsm_init_state(fsm_state_t *state) {
    state->state = fsm_state_t::fsm_socket_connected;
    state->buf = NULL;
    state->nbuf = 0;
    state->snbuf = 0;
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
