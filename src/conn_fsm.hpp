
#ifndef __CONN_FSM_HPP__
#define __CONN_FSM_HPP__

#include "containers/intrusive_list.hpp"
#include "arch/resource.hpp"
#include "request_handler/request_handler.hpp"
#include "event.hpp"
#include "btree/fsm.hpp"
#include "corefwd.hpp"

// TODO: the lifetime of conn_fsm isn't well defined - some objects
// may persist for far longer than others. The small object dynamic
// pool allocator (currently defined as alloc_t in config_t) is
// designed for objects that have roughly the same lifetime. We should
// use a different allocator for objects like conn_fsm (and btree
// buffers).

// The actual state structure
template<class config_t>
struct conn_fsm : public intrusive_list_node_t<conn_fsm<config_t> >,
                  public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, conn_fsm<config_t> >
{
public:
    typedef typename config_t::iobuf_t iobuf_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename config_t::request_t request_t;
    typedef typename config_t::req_handler_t req_handler_t;
    
public:
    // Possible transition results
    enum result_t {
        fsm_invalid,
        fsm_shutdown_server,
        fsm_quit_connection,
        fsm_transition_ok,
    };

    enum state_t {
        // Socket is connected, is in a clean state (no outstanding ops) and ready to go
        fsm_socket_connected,
        // Socket has received an incomplete packet and waiting for the rest of the command
        fsm_socket_recv_incomplete,
        // We sent a msg over the socket but were only able to send a partial packet
        fsm_socket_send_incomplete,
        // Waiting for IO initiated by the BTree to complete
        fsm_btree_incomplete
    };
    
public:
    conn_fsm(resource_t _source, event_queue_t *_event_queue);
    ~conn_fsm();
    
    result_t do_transition(event_t *event);
    void consume(unsigned int bytes);

    int get_source() {
        return source;
    }
    
#ifndef NDEBUG
    // Print debugging information designed to resolve deadlocks
    void deadlock_debug() {
        printf("conn-fsm %p {\n", this);
        const char *st_name;
        switch(state) {
            case fsm_socket_connected: st_name = "fsm_socket_connected"; break;
            case fsm_socket_recv_incomplete: st_name = "fsm_socket_recv_incomplete"; break;
            case fsm_socket_send_incomplete: st_name = "fsm_socket_send_incomplete"; break;
            case fsm_btree_incomplete: st_name = "fsm_btree_incomplete"; break;
            default: st_name = "<invalid state>"; break;
        }
        printf("\tstate = %s\n", st_name);
        if (current_request) {
            printf("\tcurrent_request.fsms = [\n");
            unsigned int i;
            for (i=0; i<current_request->nstarted; i++) {
                current_request->fsms[i]->deadlock_debug();
            }
            printf("]\n");
        } else {
            printf("\tcurrent_request = NULL\n");
        }
    }
#endif

public:
    int source;
    state_t state;

    // A buffer with IO communication (possibly incomplete). The nbuf
    // variable indicates how many bytes are stored in the buffer. The
    // snbuf variable indicates how many bytes out of the buffer have
    // been sent (in case of a send workflow).
    char *buf, *sbuf;
    unsigned int nbuf, snbuf;
    req_handler_t *req_handler;
    event_queue_t *event_queue;
    request_t *current_request;
    
private:
    result_t do_socket_ready(event_t *event);
    result_t do_socket_send_incomplete(event_t *event);
    result_t do_fsm_btree_incomplete(event_t *event);
    
    void send_msg_to_client();
    void send_err_to_client();
    void init_state();
    void return_to_socket_connected();
};

// Include the implementation
#include "conn_fsm.tcc"

#endif // __CONN_FSM_HPP__

