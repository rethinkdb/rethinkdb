
#ifndef __FSM_IMPL_HPP__
#define __FSM_IMPL_HPP__

#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include "utils.hpp"

template<class config_t>
void conn_fsm<config_t>::init_state() {
    this->state = fsm_socket_connected;
    this->buf = NULL;
    this->nbuf = 0;
    this->snbuf = 0;
}

// This function returns the socket to clean connected state
template<class config_t>
void conn_fsm<config_t>::return_to_socket_connected() {
    event_queue_t::alloc_t *alloc = tls_small_obj_alloc_accessor<event_queue_t::alloc_t>::template get_alloc<iobuf_t>();
    alloc->free(this->buf);
    init_state();
}

// This state represents a connected socket with no outstanding
// operations. Incoming events should be user commands received by the
// socket.
template<class config_t>
typename conn_fsm<config_t>::result_t conn_fsm<config_t>::do_socket_ready(event_t *event) {
    size_t sz;
    conn_fsm *state = (conn_fsm*)event->state;

    if(event->event_type == et_sock) {
        if(state->buf == NULL) {
            event_queue_t::alloc_t *alloc = tls_small_obj_alloc_accessor<event_queue_t::alloc_t>::template get_alloc<iobuf_t>();
            state->buf = (char *)alloc->malloc(sizeof(iobuf_t));
            state->nbuf = 0;
        }
            
        // TODO: we assume the command will fit comfortably into
        // IO_BUFFER_SIZE. We'll need to implement streaming later.

        do {
            sz = iocalls_t::read(state->source,
                                 state->buf + state->nbuf,
                                 iobuf_t::size - state->nbuf);
            if(sz == -1) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    // The machine can't be in
                    // fsm_socket_send_incomplete state here,
                    // since we break out in these cases. So it's
                    // safe to free the buffer.
                    if(state->state != conn_fsm::fsm_socket_recv_incomplete)
                        return_to_socket_connected();
                    break;
                } else {
                    check("Could not read from socket", sz == -1);
                }
            } else if(sz > 0) {
                state->nbuf += sz;
                typename req_handler_t::parse_result_t handler_res =
                    req_handler->parse_request(event);
                switch(handler_res) {
                case req_handler_t::op_malformed:
                    // Command wasn't processed correctly, send error
                    send_err_to_client();
                    break;
                case req_handler_t::op_partial_packet:
                    // The data is incomplete, keep trying to read in
                    // the current read loop
                    state->state = conn_fsm::fsm_socket_recv_incomplete;
                    break;
                case req_handler_t::op_req_shutdown:
                    // Shutdown has been initiated
                    return fsm_shutdown_server;
                case req_handler_t::op_req_quit:
                    // The connection has been closed
                    return fsm_quit_connection;
                case req_handler_t::op_req_complex:
                    // Ain't nothing we can do now - the operations
                    // have been distributed accross CPUs. We can just
                    // sit back and wait until they come back.
                    assert(current_request);
                    state->state = fsm_btree_incomplete;
                    return fsm_transition_ok;
                    break;
                default:
                    check("Unknown request parse result", 1);
                }

                if(state->state == conn_fsm::fsm_socket_send_incomplete) {
                    // Wait for the socket to finish sending
                    break;
                }
            } else {
                // Socket has been closed, destroy the connection
                return fsm_quit_connection;
                    
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

    return fsm_transition_ok;
}

template<class config_t>
typename conn_fsm<config_t>::result_t conn_fsm<config_t>::do_fsm_btree_incomplete(event_t *event)
{
    if(event->event_type == et_sock) {
        // We're not going to process anything else from the socket
        // until we complete the currently executing command.

        // TODO: This strategy destroys any possibility of pipelining
        // commands on a single socket. We should enable this in the
        // future (fsm would need to associate IO responses with a
        // given command).
    } else if(event->event_type == et_request_complete) {
        send_msg_to_client();
        if(this->state != conn_fsm::fsm_socket_send_incomplete) {
            // We've finished sending completely, now see if there is
            // anything left to read from the old epoll notification,
            // and let fsm_socket_ready do the cleanup
            event->op = eo_read;
            event->event_type = et_sock;
            do_socket_ready(event);
        }
    } else {
        check("fsm_btree_incomplete: Invalid event", 1);
    }
    
    return fsm_transition_ok;
}

// The socket is ready for sending more information and we were in the
// middle of an incomplete send request.
template<class config_t>
typename conn_fsm<config_t>::result_t conn_fsm<config_t>::do_socket_send_incomplete(event_t *event) {
    // TODO: incomplete send needs to be tested therally. It's not
    // clear how to get the kernel to artifically limit the send
    // buffer.
    if(event->event_type == et_sock) {
        if(event->op == eo_rdwr || event->op == eo_write) {
            send_msg_to_client();
        }
        if(this->state != conn_fsm::fsm_socket_send_incomplete) {
            // We've finished sending completely, now see if there is
            // anything left to read from the old epoll notification,
            // and let fsm_socket_ready do the cleanup
            event->op = eo_read;
            do_socket_ready(event);
        }
    } else {
        check("fsm_socket_send_ready: Invalid event", 1);
    }
    return fsm_transition_ok;
}

// Switch on the current state and call the appropriate transition
// function.
template<class config_t>
typename conn_fsm<config_t>::result_t conn_fsm<config_t>::do_transition(event_t *event) {
    // TODO: Using parent_pool member variable within state
    // transitions might cause cache line alignment issues. Can we
    // eliminate it (perhaps by giving each thread its own private
    // copy of the necessary data)?

    result_t res;

    switch(state) {
    case fsm_socket_connected:
    case fsm_socket_recv_incomplete:
        res = do_socket_ready(event);
        break;
    case fsm_socket_send_incomplete:
        res = do_socket_send_incomplete(event);
        break;
    case fsm_btree_incomplete:
        res = do_fsm_btree_incomplete(event);
        break;
    default:
        res = fsm_invalid;
        check("Invalid state", 1);
    }

    return res;
}

template<class config_t>
conn_fsm<config_t>::conn_fsm(resource_t _source, req_handler_t *_req_handler, event_queue_t *_event_queue)
    : source(_source), req_handler(_req_handler),
      event_queue(_event_queue)
{
    init_state();
}

template<class config_t>
conn_fsm<config_t>::~conn_fsm() {
    if(this->buf) {
        event_queue_t::alloc_t *alloc = tls_small_obj_alloc_accessor<event_queue_t::alloc_t>::template get_alloc<iobuf_t>();
        alloc->free(this->buf);
    }
}

// Send a message to the client. The message should be contained
// within buf (nbuf should be the full size). If state has been
// switched to fsm_socket_send_incomplete, then buf must not be freed
// after the return of this function.
template<class config_t>
void conn_fsm<config_t>::send_msg_to_client() {
    // Either number of bytes already sent should be zero, or we
    // should be in the middle of an incomplete send.
    assert(this->snbuf == 0 || this->state == conn_fsm::fsm_socket_send_incomplete);

    int len = this->nbuf - this->snbuf;
    int sz = 0;
    do {
        this->snbuf += sz;
        len -= sz;
        
        sz = iocalls_t::write(this->source, this->buf + this->snbuf, len);
        if(sz < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                // If we can't send the message now, wait 'till we can
                this->state = conn_fsm::fsm_socket_send_incomplete;
                return;
            } else {
                // There was some other error
                check("Couldn't send message to client", sz < 0);
            }
        }
    } while(sz < len);

    // We've successfully sent everything out
    this->snbuf = 0;
    this->nbuf = 0;
    this->state = conn_fsm::fsm_socket_connected;
}

template<class config_t>
void conn_fsm<config_t>::send_err_to_client() {
    char err_msg[] = "(ERROR) Unknown command\n";
    strcpy(this->buf, err_msg);
    this->nbuf = strlen(err_msg) + 1;
    send_msg_to_client();
}

#endif // __FSM_IMPL_HPP__

