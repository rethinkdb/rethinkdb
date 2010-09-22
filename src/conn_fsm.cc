#include "conn_fsm.hpp"
#include <unistd.h>
#include <errno.h>
#include "utils.hpp"
#include "request_handler/memcached_handler.hpp"

void conn_fsm_t::init_state() {
    state = fsm_socket_connected;
    rbuf = NULL;
    sbuf = NULL;
    nrbuf = 0;
    corked = false;

    ext_rbuf = NULL;
    ext_nrbuf = 0;
    ext_size = 0;
    cb = NULL;
}

// This function returns the socket to clean connected state
void conn_fsm_t::return_to_socket_connected() {
    if(rbuf)
        delete (iobuf_t*)(rbuf);
    if(sbuf)
        delete (linked_buf_t*)(sbuf);
    init_state();
}

conn_fsm_t::result_t conn_fsm_t::fill_buf(void *buf, unsigned int *bytes_filled, unsigned int total_length) {
    // TODO: we assume the command will fit comfortably into
    // IO_BUFFER_SIZE. We'll need to implement streaming later.
    ssize_t sz = get_cpu_context()->event_queue->iosys.read(source,
            (byte *) buf + *bytes_filled,
            total_length - *bytes_filled);
    if(sz == -1) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            // The machine can't be in
            // fsm_socket_send_incomplete state here,
            // since we break out in these cases. So it's
            // safe to free the buffer.
            assert(state != fsm_socket_send_incomplete);
            //TODO Modify this so that we go into send_incomplete and try to empty our send buffer
            //this is a pretty good first TODO
            if(state != fsm_socket_recv_incomplete && *bytes_filled == 0)
                return_to_socket_connected();
            else
                state = fsm_socket_connected; //we're wating for a socket event
            //break;
        } else if (errno == ENETDOWN) {
            check("Enetdown wtf", sz == -1);
        } else {
            check("Could not read from socket", sz == -1);
        }
    } else if(sz > 0 || *bytes_filled > 0) {
        *bytes_filled += sz;
        if (state != fsm_socket_recv_incomplete)
            state = fsm_outstanding_data;
    } else {
        if (state == fsm_socket_recv_incomplete)
            return fsm_no_data_in_socket;
        else
            return fsm_quit_connection;
        // TODO: what about application-level keepalive?
    }

    return fsm_transition_ok;
}

// This state represents a connected socket with no outstanding
// operations. Incoming events should be user commands received by the
// socket.
conn_fsm_t::result_t conn_fsm_t::fill_rbuf() {
    if(rbuf == NULL) {
        rbuf = (char *) new iobuf_t();
        nrbuf = 0;
    }
    if(sbuf == NULL) {
        sbuf = new linked_buf_t();
    }
    return fill_buf(rbuf, &nrbuf, iobuf_t::size);
}

conn_fsm_t::result_t conn_fsm_t::fill_ext_rbuf() {
    assert(ext_rbuf);
    assert(nrbuf == 0);
    return fill_buf(ext_rbuf, &ext_nrbuf, ext_size);
}

conn_fsm_t::result_t conn_fsm_t::read_data(event_t *event) {
    // TODO: this is really silly; this notification should be done differently
    assert((conn_fsm_t *) event->state == this);

    if (ext_rbuf) {
        result_t res = fill_ext_rbuf();
        check_external_buf();
        return res;
    } else {
        return fill_rbuf();
    }
}

void conn_fsm_t::fill_external_buf(byte *external_buf, unsigned int size, data_transferred_callback *callback) {
    assert(!ext_rbuf && ext_nrbuf == 0 && ext_size == 0 && !cb);
    //assert(!ext_rbuf && ext_nrbuf == 0 && ext_size == 0);// && !cb);

    ext_rbuf = external_buf;
    ext_size = size;
    cb = callback;

    //previous_state = state;

    unsigned int bytes_to_move = ext_size < nrbuf ? ext_size : nrbuf;
    memcpy(ext_rbuf, rbuf, bytes_to_move);
    consume(bytes_to_move);
    //nrbuf -= bytes_to_move;
    ext_nrbuf = bytes_to_move;
    check_external_buf();
    dummy_sock_event();
}

void conn_fsm_t::send_external_buf(byte *external_buf, unsigned int size, data_transferred_callback *callback) {
    // TODO: Write the data directly to the socket instead of to the sbuf when the request handler has better support for it.
    sbuf->append(external_buf, size);
    callback->on_data_transferred();
}

void conn_fsm_t::dummy_sock_event() {
    event_t event;
    bzero((void*)&event, sizeof(event));
    event.event_type = et_sock;
    event.state = this;
    do_transition(&event);
}

void conn_fsm_t::check_external_buf() {
    assert(ext_rbuf && ext_nrbuf <= ext_size);
    if (ext_nrbuf < ext_size) {
        state = fsm_socket_recv_incomplete; // XXX Only do this in do_fsm_outstanding_req?
    } else {
        //state = previous_state;
        state = fsm_outstanding_data; // XXX
        assert(cb);
        data_transferred_callback *_cb = cb;
        ext_rbuf = NULL;
        ext_nrbuf = 0;
        ext_size = 0;
        cb = NULL;
        _cb->on_data_transferred();
    }
}

conn_fsm_t::result_t conn_fsm_t::do_fsm_btree_incomplete(event_t *event) {
    if(event->event_type == et_sock) {
        // We're not going to process anything else from the socket
        // until we complete the currently executing command.
    } else if(event->event_type == et_request_complete) {
        send_msg_to_client();
        if(state != fsm_socket_send_incomplete) {
            state = fsm_outstanding_data;
        }
    } else {
        fail("fsm_btree_incomplete: Invalid event");
    }
    
    return fsm_transition_ok;
}

// The socket is ready for sending more information and we were in the
// middle of an incomplete send request.
conn_fsm_t::result_t conn_fsm_t::do_socket_send_incomplete(event_t *event) {
    // TODO: incomplete send needs to be tested therally. It's not
    // clear how to get the kernel to artifically limit the send
    // buffer.
    if (event->event_type == et_sock) {
        if(event->op == eo_rdwr || event->op == eo_write) {
            send_msg_to_client();
        }
        if(state != fsm_socket_send_incomplete) {
            state = fsm_outstanding_data;
        }
    } else {
        fail("fsm_socket_send_ready: Invalid event");
    }
    return fsm_transition_ok;
}

conn_fsm_t::result_t conn_fsm_t::do_fsm_outstanding_req(event_t *event) {
    //We've processed a request but there are still outstanding requests in our rbuf
    assert((conn_fsm_t*) event->state == this);
    if (ext_rbuf) {
        check_external_buf();
        return fsm_transition_ok;
    }

    if (nrbuf == 0) {
        state = fsm_socket_recv_incomplete;
        return fsm_transition_ok;
    }

    request_handler_t::parse_result_t handler_res = req_handler->parse_request(event);
    switch(handler_res) {
        case request_handler_t::op_malformed:
            // Command wasn't processed correctly, send error
            // Error should already be placed in buffer by parser
            send_msg_to_client();
            state = fsm_outstanding_data;
            break;
        case request_handler_t::op_partial_packet:
            // The data is incomplete, keep trying to read in
            // the current read loop
            state = fsm_socket_recv_incomplete;
            break;
        case request_handler_t::op_req_shutdown:
            // Shutdown has been initiated
            return fsm_shutdown_server;
        case request_handler_t::op_req_quit:
            // The connection has been closed
            return fsm_quit_connection;
        case request_handler_t::op_req_complex:
            // Ain't nothing we can do now - the operations
            // have been distributed accross CPUs. We can just
            // sit back and wait until they come back.
            state = fsm_btree_incomplete;
            return fsm_transition_ok;
            break;
        case request_handler_t::op_req_parallelizable:
            state = fsm_outstanding_data;
            return fsm_transition_ok;
            break;
        case request_handler_t::op_req_send_now:
            send_msg_to_client();
            state = fsm_outstanding_data;
            return fsm_transition_ok;
        default:
            fail("Unknown request parse result");
    }
    return fsm_transition_ok;
}

// Switch on the current state and call the appropriate transition
// function.
conn_fsm_t::result_t conn_fsm_t::do_transition(event_t *event) {
    // TODO: Using parent_pool member variable within state
    // transitions might cause cache line alignment issues. Can we
    // eliminate it (perhaps by giving each thread its own private
    // copy of the necessary data)?
    result_t res;

    switch (state) {
        case fsm_socket_connected:
        case fsm_socket_recv_incomplete:
            res = read_data(event);
            break;
        case fsm_socket_send_incomplete:
            res = do_socket_send_incomplete(event);
            break;
        case fsm_btree_incomplete:
            res = do_fsm_btree_incomplete(event);
            break;
        case fsm_outstanding_data:
            res = fsm_transition_ok;
            break;
        default:
            res = fsm_invalid;
            fail("Invalid state");
    }
    if (state == fsm_outstanding_data && res != fsm_quit_connection && res != fsm_shutdown_server) {
        if (nrbuf == 0) {
            //fill up the buffer
            event->event_type = et_sock;
            res = read_data(event);
        }
        if (state != fsm_outstanding_data)
            return res;
        //there's still data in our rbuf, deal with it
        //this is awkward, but we need to make sure that we loop here until we
        //actually create a btree request
        do {
#ifdef MEMCACHED_STRICT
            bool was_corked = corked;
#endif
            res = do_fsm_outstanding_req(event);
            if (res == fsm_shutdown_server || res == fsm_quit_connection) {
                return_to_socket_connected();
                return res;
            }

            if (state == fsm_socket_recv_incomplete) {
                event->event_type = et_sock;
                res = read_data(event);
                
                if (res == fsm_no_data_in_socket)
                    return fsm_transition_ok;
            }

#ifdef MEMCACHED_STRICT
            if (was_corked && !corked)
                send_msg_to_client();
#endif
        } while (state == fsm_socket_recv_incomplete || state == fsm_outstanding_data);
    }

    return res;
}

conn_fsm_t::conn_fsm_t(resource_t _source, event_queue_t *_event_queue)
    : source(_source), req_handler(NULL), event_queue(_event_queue)
{
    req_handler = new memcached_handler_t(event_queue, this);
    init_state();
}

conn_fsm_t::~conn_fsm_t() {
    close(source);
    delete req_handler;
    if (rbuf) {
        delete (iobuf_t*)(rbuf);
    }
    if (sbuf) {
        delete (sbuf);
    }
}

// Send a message to the client. The message should be contained
// within sbuf (nbuf should be the full size). If state has been
// switched to fsm_socket_send_incomplete, then buf must not be freed
// after the return of this function.
void conn_fsm_t::send_msg_to_client() {
    // Either number of bytes already sent should be zero, or we
    // should be in the middle of an incomplete send.
    //assert(snbuf == 0 || state == conn_fsm::fsm_socket_send_incomplete); TODO equivalent thing for seperate buffers

    if (!corked) {
        int res = sbuf->send(source);
        if (sbuf->gc_me)
            sbuf = sbuf->garbage_collect();

        switch (res) {
            case linked_buf_t::linked_buf_outstanding:
                state = fsm_socket_send_incomplete;
                break;
            case linked_buf_t::linked_buf_empty:
                state = fsm_outstanding_data;
                break;
        }
    }
}

void conn_fsm_t::consume(unsigned int bytes) {
    assert (bytes <= nrbuf);
    memmove(rbuf, rbuf + bytes, nrbuf - bytes);
    nrbuf -= bytes;
}
