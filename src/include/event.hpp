
#ifndef __EVENT_HPP__
#define __EVENT_HPP__

#include "arch/resource.hpp"

// TODO: the way we have conn_fsm_t derive from event_state_t is
// stupid. We should change this to have event_state_t compose proper
// state that we need, so we can get it out. Casting event_state_t to
// a connection state machine is 1) ackward, 2) doesn't let us add
// additional state.

// Event
struct event_state_t {
    event_state_t(resource_t _source) : source(_source) {}
    resource_t source;
};

enum event_type_t {
    et_disk, et_sock, et_timer
};
enum event_op_t {
    eo_read, eo_write, eo_rdwr
};
struct event_t {
    event_type_t event_type;
    event_op_t op;

    // State associated with the communication (must have been passed
    // to watch_resource).
    event_state_t *state;

    /* For event_type == et_disk_event, contains the result of the IO
     * operation. For event_type == et_timer_event, contains the
     * number of experiations of the timer that have occurred. */
    int result;

    /* For event_type == et_disk_event */
    void *buf;        // Location of the buffer
    off64_t offset;   // Offset into the file
};

#endif // __EVENT_HPP__

