
#include <assert.h>
#include <unistd.h>
#include "event_queue.hpp"
#include "fsm.hpp"
#include "worker_pool.hpp"

// TODO: we should refactor the FSM to be able to unit test state
// transitions independant of the OS network subsystem (via mock-ups,
// or via turning the code 'inside-out' in a Haskell sense).

void fsm_init_state(fsm_state_t *state) {
    state->state = fsm_state_t::fsm_socket_connected;
    state->buf = NULL;
    state->nbuf = 0;
    state->snbuf = 0;
}

// This function returns the socket to clean connected state
void return_to_fsm_socket_connected(small_obj_alloc_t *alloc, fsm_state_t *state) {
    alloc->free((io_buffer_t*)state->buf);
    fsm_init_state(state);
}

// This state represents a connected socket with no outstanding
// operations. Incoming events should be user commands received by the
// socket.
int fsm_state_t::do_socket_ready(event_t *event) {
    int res;
    size_t sz;
    fsm_state_t *state = (fsm_state_t*)event->state;

    if(event->event_type == et_sock) {
        if(state->buf == NULL) {
            state->buf = (char*)alloc->malloc<io_buffer_t>();
            state->nbuf = 0;
        }
            
        // TODO: we assume the command will fit comfortably into
        // IO_BUFFER_SIZE. We'll need to implement streaming later.

        do {
            sz = read(state->source,
                      state->buf + state->nbuf,
                      IO_BUFFER_SIZE - state->nbuf);
            if(sz == -1) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    // The machine can't be in
                    // fsm_socket_send_incomplete state here,
                    // since we break out in these cases. So it's
                    // safe to free the buffer.
                    if(state->state != fsm_state_t::fsm_socket_recv_incomplete)
                        return_to_fsm_socket_connected(alloc, state);
                    break;
                } else {
                    check("Could not read from socket", sz == -1);
                }
            } else if(sz > 0) {
                state->nbuf += sz;
                res = process_command(event, btree);
                if(res == -1 || res == 0) {
                    if(res == -1) {
                        // Command wasn't processed correctly, send error
                        send_err_to_client(state);
                    }
                    if(state->state != fsm_state_t::fsm_socket_send_incomplete) {
                        // Command is either completed or malformed, in any
                        // case get back to clean connected state
                        state->state = fsm_state_t::fsm_socket_connected;
                        state->nbuf = 0;
                        state->snbuf = 0;
                    } else {
                        // Wait for the socket to finish sending
                        break;
                    }
                } else if(res == 1) {
                    state->state = fsm_state_t::fsm_socket_recv_incomplete;
                } else if(res == 2) {
                    // The connection has been closed
                    return 2;
                } else if(res == 3) {
                    // Shutdown has been initiated
                    return 1;
                }
            } else {
                // Socket has been closed, destroy the connection
                return 2;
                    
                // TODO: what if the fsm is not in a finished
                // state? What if we free it during an AIO
                // request, and the AIO request comes back? We
                // need an fsm_terminated flag for these cases.

                // TODO: what about application-level keepalive?
            }
        } while(1);
    } else {
        check("fsm_socket_ready: Invalid event", 1);
    }

    return 0;
}

// The socket is ready for sending more information and we were in the
// middle of an incomplete send request.
int fsm_state_t::do_socket_send_incomplete(event_t *event) {
    // TODO: incomplete send needs to be tested therally. It's not
    // clear how to get the kernel to artifically limit the send
    // buffer.
    if(event->event_type == et_sock) {
        fsm_state_t *state = (fsm_state_t*)event->state;
        if(event->op == eo_rdwr || event->op == eo_write) {
            send_msg_to_client(state);
        }
        if(state->state != fsm_state_t::fsm_socket_send_incomplete) {
            // We've finished sending completely, now see if there is
            // anything left to read from the old epoll notification,
            // and let fsm_socket_ready do the cleanup
            event->op = eo_read;
            do_socket_ready(event);
        }
    } else {
        check("fsm_socket_send_ready: Invalid event", 1);
    }
    return 0;
}

// Switch on the current state and call the appropriate transition
// function.
int fsm_state_t::do_transition(event_t *event) {
    // TODO: Using parent_pool member variable within state
    // transitions might cause cache line alignment issues. Can we
    // eliminate it (perhaps by giving each thread its own private
    // copy of the necessary data)?

    int res;

    switch(state) {
    case fsm_state_t::fsm_socket_connected:
    case fsm_state_t::fsm_socket_recv_incomplete:
        res = do_socket_ready(event);
        break;
    case fsm_state_t::fsm_socket_send_incomplete:
        res = do_socket_send_incomplete(event);
        break;
    default:
        res = -1;
        check("Invalid state", 1);
    }

    return res;
}

fsm_state_t::fsm_state_t(resource_t _source, rethink_tree_t *_btree,
                         small_obj_alloc_t* _alloc)
    : event_state_t(_source), btree(_btree), alloc(_alloc)
{
    fsm_init_state(this);
}

fsm_state_t::~fsm_state_t() {
    if(this->buf) {
        alloc->free((io_buffer_t*)this->buf);
    }
}

