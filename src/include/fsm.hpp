
#ifndef __FSM_HPP__
#define __FSM_HPP__

#include "arch/common.hpp"
#include "containers/intrusive_list.hpp"
#include "common.hpp"

struct event_state_t {
    event_state_t(resource_t _source) : source(_source) {}
    resource_t source;
};

// The states are collected via an intrusive list
struct fsm_state_t;
typedef intrusive_list_node_t<fsm_state_t> fsm_list_node_t;
typedef intrusive_list_t<fsm_state_t> fsm_list_t;

// Define the state structure
struct event_queue_t;
struct event_t;

struct fsm_state_t : public event_state_t, public fsm_list_node_t {
    fsm_state_t(event_queue_t *_event_queue, resource_t _source);
    ~fsm_state_t();
    
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
    event_queue_t *event_queue;
};

void fsm_do_transition(event_queue_t *event_queue, event_t *event);

#endif // __FSM_HPP__

