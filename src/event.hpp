#ifndef __EVENT_HPP__
#define __EVENT_HPP__

#include "arch/resource.hpp"
#include <sys/types.h>

// Event
enum event_type_t {
    et_empty, et_sock, et_request_complete, et_cache, et_large_buf,
    et_cpu_event, et_commit
};
enum event_op_t {
    eo_read, eo_write, eo_rdwr
};
struct event_t {
    /* TODO(NNW): We should make a constructor for this type. */
    event_type_t event_type;
    event_op_t op;

    // State associated with the communication (must have been passed
    // to watch_resource).
    void *state;

    /* Contains the result of the operation. For event_type ==
     * et_timer_event, contains the number of experiations of the
     * timer that have occurred. */
    int result;

    void *buf;        // Location of the buffer
    off64_t offset;   // Offset into the file
};

#endif // __EVENT_HPP__
