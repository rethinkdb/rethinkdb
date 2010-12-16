#ifndef __EVENT_HPP__
#define __EVENT_HPP__

#include <sys/types.h>
#include <stdio.h>

/* TODO: It would be nice to remove this file and the event_t type completely. Right now, it's used
irregularly in a couple of different places, but it doesn't really mean anything and should just
be gotten rid of. */

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
    void print() {
#ifndef NDEBUG
        printf("           Event        \n");
        printf("------------------------\n|");
        switch (event_type) {
        case et_empty:
            printf("et_empty\t\t");
            break;
        case et_sock:
            printf("et_sock\t\t");
            break;
        case et_request_complete:
            printf("et_request_complete\t");
            break;
        case et_cache:
            printf("et_cache\t\t");
            break;
        case et_large_buf:
            printf("et_large_buf\t");
            break;
        case et_cpu_event:
            printf("et_cpu_event\t");
            break;
        case et_commit:
            printf("et_commit\t");
            break;
        default:
            printf("INVALID\t");
            break;
        }
        printf("|\n");
        printf("------------------------\n|");
        switch(op) {
        case eo_read:
            printf("eo_read\t\t");
            break;
        case eo_write:
            printf("eo_write\t\t");
            break;
        case eo_rdwr:
            printf("eo_rdwr\t\t");
            break;
        default:
            printf("INVALID");
        }
        printf("|\n");
        printf("------------------------\n");
#endif
    }
};

#endif // __EVENT_HPP__
