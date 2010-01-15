
#ifndef __FSM_HPP__
#define __FSM_HPP__

#include "arch/common.hpp"

struct event_state_t {
    resource_t source;
};

struct fsm_state_t : public event_state_t {
    enum state_t {
        fsm_socket_connected
    };
    state_t state;

    // TODO: Remove this - this is added to meet minimum size requirements of the allocator
    char temp[8];
};

struct event_queue_t;
struct event_t;

void fsm_init_state(fsm_state_t *state);
void fsm_do_transition(event_queue_t *event_queue, event_t *event);

#endif // __FSM_HPP__

