
#ifndef __FSM_HPP__
#define __FSM_HPP__

#include "containers/intrusive_list.hpp"
#include "arch/common.hpp"
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
struct event_t;

struct fsm_state_t : public event_state_t, public fsm_list_node_t {
    fsm_state_t(resource_t _source, rethink_tree_t *_btree,
                small_obj_alloc_t* _alloc);
    ~fsm_state_t();
    
    int do_transition(event_t *event);

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
    rethink_tree_t *btree;
    small_obj_alloc_t *alloc;
private:
    int do_socket_ready(event_t *event);
    int do_socket_send_incomplete(event_t *event);
};

// Some internal functions
void send_msg_to_client(fsm_state_t *state);
void send_err_to_client(fsm_state_t *state);
int process_command(event_t *event, rethink_tree_t *btree);

#endif // __FSM_HPP__

