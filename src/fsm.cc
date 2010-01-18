
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "event_queue.hpp"
#include "worker_pool.hpp"
#include "fsm.hpp"
#include "utils.hpp"

// This function returns the socket to clean connected state
void return_to_fsm_socket_connected(event_queue_t *event_queue, fsm_state_t *state) {
    state->state = fsm_state_t::fsm_socket_connected;
    event_queue->alloc.free((io_buffer_t*)state->buf);
    state->buf = NULL;
    state->nbuf = 0;
}

// Send a message to the client. The message should be contained
// within state->buf (state->nbuf should be the full size). If state
// has been switched to fsm_socket_send_incomplete, then state->buf
// must not be freed after the return of this function.
void send_msg_to_client(event_queue_t *event_queue, fsm_state_t *state) {
    // Either number of bytes already sent should be zero, or we
    // should be in the middle of an incomplete send.
    assert(state->snbuf == 0 || state->state == fsm_state_t::fsm_socket_send_incomplete);

    int len = state->nbuf - state->snbuf;
    int sz = 0;
    do {
        state->snbuf += sz;
        len -= sz;
        
        sz = write(state->source, state->buf + state->snbuf, len);
        if(sz < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                // If we can't send the message now, wait 'till we can
                state->state = fsm_state_t::fsm_socket_send_incomplete;
                return;
            } else {
                // There was some other error
                check("Couldn't send message to client", sz < 0);
            }
        }
    } while(sz < len);

    // We've successfully sent everything out
    state->snbuf = 0;
}

// Process commands received from the user
int process_command(event_queue_t *event_queue, event_t *event) {
    int res;

    fsm_state_t *state = (fsm_state_t*)event->state;
    char *buf = state->buf;
    unsigned int size = state->nbuf;

    // TODO: if we get incomplete packets, we retokenize the entire
    // message every time we get a new piece of the packet. Perhaps it
    // would be more efficient to store tokenizer state across
    // requests. In general, tokenizing a request is silly, we should
    // really provide a binary API.
    
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
        // Make sure there's no more tokens
        if((token = tokenize(token + token_size,
                             buf + size - (token + token_size),
                             delims, &token_size)) != NULL) {
            return -1;
        }
        // Quit the connection (the fsm will be freed when "socket
        // closed" message hits epoll)
        close(state->source);
        state->source = -1;
    } else if(token_size == 8 && strncmp(token, "shutdown", 8) == 0) {
        // Make sure there's no more tokens
        if((token = tokenize(token + token_size,
                             buf + size - (token + token_size),
                             delims, &token_size)) != NULL) {
            return -1;
        }
        // Shutdown the server
        printf("Shutting down server...\n");
        res = pthread_kill(event_queue->parent_pool->main_thread, SIGINT);
        check("Could not send kill signal to main thread", res != 0);
    } else {
        char err_msg[] = "(ERROR) Unknown command\n";
        // Since we're in the middle of processing a command,
        // state->buf must exist at this point.
        strcpy(state->buf, err_msg);
        state->nbuf = strlen(err_msg) + 1;
        send_msg_to_client(event_queue, state);
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
