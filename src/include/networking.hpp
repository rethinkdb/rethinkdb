
#ifndef __NETWORK_HPP__
#define __NETWORK_HPP__

#include "event_queue.hpp"
#include "fsm.hpp"

void send_msg_to_client(event_queue_t *event_queue, fsm_state_t *state);
void send_err_to_client(event_queue_t *event_queue, fsm_state_t *state);


#endif // __NETWORK_HPP__

