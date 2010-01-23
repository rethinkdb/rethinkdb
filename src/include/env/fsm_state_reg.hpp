
#ifndef __FSM_STATE_REG_HPP__
#define __FSM_STATE_REG_HPP__

#include "event_queue.hpp"

struct fsm_state_registration_t {
    void register_fsm(fsm_state_t *state, void *arg) {
        event_queue_t *event_queue = (event_queue_t*)arg;
        event_queue->live_fsms.push_back(state);
        event_queue->watch_resource(state->source, eo_rdwr, state);
    }
    void deregister_fsm(fsm_state_t *state, void *arg) {
        event_queue_t *event_queue = (event_queue_t*)arg;
        event_queue->forget_resource(state->source);
        event_queue->live_fsms.remove(state);
    }
};

#endif // __FSM_STATE_REG_HPP__

