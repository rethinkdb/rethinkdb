
#include <unistd.h>
#include "networking.hpp"

// Send a message to the client. The message should be contained
// within state->buf (state->nbuf should be the full size). If state
// has been switched to fsm_socket_send_incomplete, then state->buf
// must not be freed after the return of this function.
void send_msg_to_client(event_queue_t *event_queue, fsm_state_t *state) {
    // Either number of bytes already sent should be zero, or we
    // should be in the middle of an incomplete send.
    assert(state->snbuf == 0 || state->state == fsm_state_t::fsm_socket_send_incomplete);

    int len = state->nbuf - state->snbuf;
    int sz = 0;
    do {
        state->snbuf += sz;
        len -= sz;
        
        sz = write(state->source, state->buf + state->snbuf, len);
        if(sz < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                // If we can't send the message now, wait 'till we can
                state->state = fsm_state_t::fsm_socket_send_incomplete;
                return;
            } else {
                // There was some other error
                check("Couldn't send message to client", sz < 0);
            }
        }
    } while(sz < len);

    // We've successfully sent everything out
    state->snbuf = 0;
}

void send_err_to_client(event_queue_t *event_queue, fsm_state_t *state) {
    char err_msg[] = "(ERROR) Unknown command\n";
    strcpy(state->buf, err_msg);
    state->nbuf = strlen(err_msg) + 1;
    send_msg_to_client(event_queue, state);
}

