#include "conn_fsm.hpp"
#include <unistd.h>
#include <errno.h>
#include "utils.hpp"
#include <signal.h>

/* Global counters for the number of conn_fsms in each state */

static perfmon_duration_sampler_t
    pm1("conns_in_socket_connected", secs_to_ticks(1)),
    pm2("conns_in_socket_recv_incomplete", secs_to_ticks(1)),
    pm3("conns_in_socket_send_incomplete", secs_to_ticks(1)),
    pm4("conns_in_btree_incomplete", secs_to_ticks(1)),
    pm5("conns_in_outstanding_data", secs_to_ticks(1));
static perfmon_duration_sampler_t *state_counters[] = { &pm1, &pm2, &pm3, &pm4, &pm5 };

/*
~~~ A Brief History of the conn_fsm_t ~~~

Initially, the event queue would call conn_fsm_t::do_transition() and
then examine the result to figure out what to do. Also, the conn_fsm
was responsible for creating the request handler.

When Tim abstracted the IO layer, then the events would come through
conn_fsm_t::on_net_conn_* instead; however, he kept do_transition()
because he didn't want to rewrite the whole conn_fsm_t.

So event_t and conn_fsm_t::result_t are actually more or less obsolete.

When Tim added server_t and split up the worker_t, he made the
conn_acceptor_t create the request handler and pass it to the conn_fsm.
He also created a conn_fsm-like type called a conn_fsm_handler_t.
Perhaps they should be merged.

TODO: Rewrite this shit.
*/

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

/* !< return the function to a clean state.
   if error == false make sure everything is in a tidy state first
   if error == true don't worry about it
*/
void conn_fsm_t::return_to_socket_connected(bool error = false) {
    if (!error) {
        assert(nrbuf == 0);
        assert(!sbuf || sbuf->outstanding() == linked_buf_t::linked_buf_empty);
    }
    if(rbuf)
        delete (iobuf_t*)(rbuf);
    if(sbuf)
        delete (linked_buf_t*)(sbuf);
    init_state();
}

conn_fsm_t::result_t conn_fsm_t::fill_buf(void *buf, unsigned int *bytes_filled, unsigned int total_length) {
    
    // TODO: we assume the command will fit comfortably into
    // IO_BUFFER_SIZE. We'll need to implement streaming later.
    assert(!we_are_closed);
    ssize_t sz = conn->read_nonblocking((byte *) buf + *bytes_filled, total_length - *bytes_filled);
    
    if (sz == -1) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            // The machine can't be in
            // fsm_socket_send_incomplete state here,
            // since we break out in these cases. So it's
            // safe to free the buffer.
            assert(state != fsm_socket_send_incomplete);
            //TODO Modify this so that we go into send_incomplete and try to empty our send buffer
            if(state != fsm_socket_recv_incomplete && *bytes_filled == 0)
                return_to_socket_connected();
            else
                state = fsm_socket_connected; //we're wating for a socket event
            //break;
        } else if (errno == ENETDOWN) {
            check("Enetdown wtf", sz == -1);
        } else if (errno == ECONNRESET) {
#ifndef NDEBUG
            we_are_closed = true;
#endif
            if (shutdown_callback && !quitting)
                shutdown_callback->on_conn_fsm_quit();
            assert(state == fsm_outstanding_data || state == fsm_socket_connected);
            return fsm_quit_connection;
        } else {
            check("Could not read from socket", sz == -1);
        }
    } else if (sz > 0) {
        *bytes_filled += sz;
        if (state != fsm_socket_recv_incomplete)
            state = fsm_outstanding_data;
    } else {
        // TODO: process all outstanding ops already in the buffer
        // The client closed the socket, we got on_net_conn_readable()
        assert(sz == 0);
#ifndef NDEBUG
        we_are_closed = true;
#endif
        if (shutdown_callback && !quitting)
            shutdown_callback->on_conn_fsm_quit();
        assert(state == fsm_outstanding_data
               || state == fsm_socket_connected
               || state == fsm_socket_recv_incomplete);
        return fsm_quit_connection;
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
        sbuf = new linked_buf_t(); //TODO This line is a source of inefficiency, we should only delete the sbuf if the connection sits idle for a while
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

    unsigned int bytes_to_move = ext_size < nrbuf ? ext_size : nrbuf;
    memcpy(ext_rbuf, rbuf, bytes_to_move);
    consume(bytes_to_move);
    //nrbuf -= bytes_to_move;
    ext_nrbuf = bytes_to_move;
    check_external_buf();
    dummy_sock_event(); // TODO: Figure this out once quit/shutdown is fixed.
}

void conn_fsm_t::send_external_buf(const byte *external_buf, unsigned int size, data_transferred_callback *callback) {
    // TODO: Write the data directly to the socket instead of to the sbuf when the request handler has better support for it.
    sbuf->append(external_buf, size);
    callback->on_data_transferred();
}

void conn_fsm_t::dummy_sock_event() {
    event_t event;
    bzero((void*)&event, sizeof(event));
    event.event_type = et_sock;
    event.state = this;
    do_transition(&event); // TODO: Figure this out once quit/shutdown is fixed.
    //do_transition_and_handle_result(&event);
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
        state = fsm_socket_connected;
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
        case request_handler_t::op_req_quit:
            // The connection has been closed
            if (shutdown_callback && !quitting)
                shutdown_callback->on_conn_fsm_quit();
            if (state == fsm_socket_send_incomplete || state == fsm_btree_incomplete) {
                quitting = true;
                return fsm_transition_ok;
            } else {
                return fsm_quit_connection;
            }
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

void conn_fsm_t::on_net_conn_readable() {
    event_t e;
    bzero(&e, sizeof(event_t));
    e.event_type = et_sock;
    e.state = this;
    e.op = eo_read;
    do_transition_and_handle_result(&e);
}

void conn_fsm_t::on_net_conn_writable() {
    event_t e;
    bzero(&e, sizeof(event_t));
    e.event_type = et_sock;
    e.state = this;
    e.op = eo_write;
    do_transition_and_handle_result(&e);
}

void conn_fsm_t::on_net_conn_close() {
    conn = NULL;
    start_quit();
}

void conn_fsm_t::do_transition_and_handle_result(event_t *event) {
    
    int old_state = state;
    
    switch (do_transition(event)) {
        
        case fsm_transition_ok:
        case fsm_no_data_in_socket:
            if (state != old_state) {
                state_counters[old_state]->end(&start_time);
                state_counters[state]->begin(&start_time);
            }
            // No action
            break;
            
        case fsm_quit_connection:
            state_counters[old_state]->end(&start_time);
            delete this;
            break;
        
        default: fail("Unhandled fsm transition result");
    }
}

// Switch on the current state and call the appropriate transition
// function.
conn_fsm_t::result_t conn_fsm_t::do_transition(event_t *event) {
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
    if (state == fsm_outstanding_data && res != fsm_quit_connection) {
        if (nrbuf == 0) {
            //fill up the buffer
            event->event_type = et_sock;
            res = read_data(event);
            if (res == fsm_quit_connection) return res;
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
            if (!quitting)
                res = do_fsm_outstanding_req(event);
            else
                res = fsm_quit_connection;

            if (res == fsm_quit_connection) {
                consume(nrbuf);
                return_to_socket_connected();
                return res;
            }

            if ((state == fsm_socket_connected || state == fsm_socket_recv_incomplete) && res == fsm_transition_ok) {
                event->event_type = et_sock;
                res = read_data(event);

                if (res == fsm_quit_connection)
                    return res;
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

conn_fsm_t::conn_fsm_t(net_conn_t *conn, conn_fsm_shutdown_callback_t *c, request_handler_t *rh)
    : quitting(false), conn(conn), req_handler(rh), shutdown_callback(c)
{
#ifndef NDEBUG
    we_are_closed = false;
    fprintf(stderr, "Opened socket %p\n", this);
#endif
    
    conn->set_callback(this);   // I can haz chezborger when there is data on the network?
    
    init_state();
    
    state_counters[state]->begin(&start_time);
}

conn_fsm_t::~conn_fsm_t() {
    
#ifndef NDEBUG
    fprintf(stderr, "Closed socket %p\n", this);
#endif
    
    if (conn) delete conn;
    
    delete req_handler;
    
    if (rbuf) {
        delete (iobuf_t*)(rbuf);
    }
    
    if (sbuf) {
        delete (sbuf);
    }
    
    if (shutdown_callback)
        shutdown_callback->on_conn_fsm_shutdown();
}

void conn_fsm_t::start_quit() {
    
    if (quitting) return;
    
    if (shutdown_callback)
        shutdown_callback->on_conn_fsm_quit();
    
    quitting = true;

    switch (state) {
        //Fallthrough intentional
        case fsm_socket_connected:
        case fsm_socket_recv_incomplete:
        case fsm_outstanding_data:
            consume(nrbuf);
            return_to_socket_connected();
            state_counters[state]->end(&start_time);
            delete this;
            break;
        case fsm_socket_send_incomplete:
        case fsm_btree_incomplete:
            break;
        default:
            fail("Bad state");
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
        int res = sbuf->send(conn);
        if (sbuf->gc_me)
            sbuf = sbuf->garbage_collect();

        switch (res) {
            case linked_buf_t::linked_buf_outstanding:
                state = fsm_socket_send_incomplete;
                break;
            case linked_buf_t::linked_buf_empty:
                state = fsm_outstanding_data;
                break;
            case linked_buf_t::linked_buf_error:
                return_to_socket_connected(true);
                break;
            default:
                fail("Illegal value in res");
                break;
        }
    }
}

void conn_fsm_t::consume(unsigned int bytes) {
    assert (bytes <= nrbuf);
    memmove(rbuf, rbuf + bytes, nrbuf - bytes);
    nrbuf -= bytes;
}
