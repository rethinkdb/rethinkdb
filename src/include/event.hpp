
#ifndef __EVENT_HPP__
#define __EVENT_HPP__

#include "arch/resource.hpp"

// Foward declarations
struct event_queue_t;

// Event
enum event_type_t {
    et_empty, et_disk, et_sock, et_timer, et_request_complete, et_cache,
    et_cpu_event, et_commit
};
enum event_op_t {
    eo_read, eo_write, eo_rdwr
};
struct event_t {
    event_type_t event_type;
    event_op_t op;

    // State associated with the communication (must have been passed
    // to watch_resource).
    void *state;

    /* For event_type == et_disk_event, contains the result of the IO
     * operation. For event_type == et_timer_event, contains the
     * number of experiations of the timer that have occurred. */
    int result;

    /* For event_type == et_disk_event */
    void *buf;        // Location of the buffer
    off64_t offset;   // Offset into the file
};

#endif // __EVENT_HPP__

