
#ifndef __FSM_HPP__
#define __FSM_HPP__

#include "containers/intrusive_list.hpp"
#include "arch/common.hpp"
#include "common.hpp"
#include "operations.hpp"
#include "event.hpp"

// The states are collected via an intrusive list
typedef intrusive_list_node_t<rethink_fsm_t> fsm_list_node_t;
typedef intrusive_list_t<rethink_fsm_t> fsm_list_t;

// Define the state structure
struct event_t;

enum fsm_result_t {
    fsm_invalid,
    fsm_shutdown_server,
    fsm_quit_connection,
    fsm_transition_ok,
};

struct event_queue_t;

template<class io_calls_t, class alloc_t>
struct fsm_state_t : public event_state_t, public fsm_list_node_t, public io_calls_t {
    fsm_state_t(resource_t _source, alloc_t* _alloc, operations_t *_ops);
    ~fsm_state_t();
    
public:
    fsm_result_t do_transition(event_t *event);

public:
    enum state_t {
        // Socket is connected, is in a clean state (no outstanding ops) and ready to go
        fsm_socket_connected,
        // Socket has received an incomplete packet and waiting for the rest of the command
        fsm_socket_recv_incomplete,
        // We sent a msg over the socket but were only able to send a partial packet
        fsm_socket_send_incomplete
    };
    state_t state;

    // A buffer with IO communication (possibly incomplete). The nbuf
    // variable indicates how many bytes are stored in the buffer. The
    // snbuf variable indicates how many bytes out of the buffer have
    // been sent (in case of a send workflow).
    char *buf;
    unsigned int nbuf, snbuf;
    alloc_t *alloc;
    operations_t *operations;
    event_queue_t *event_queue;
private:
    fsm_result_t do_socket_ready(event_t *event);
    fsm_result_t do_socket_send_incomplete(event_t *event);
    void send_msg_to_client();
    void send_err_to_client();
    void init_state();
    void return_to_socket_connected();
};

#include "fsm_impl.hpp"

#endif // __FSM_HPP__

