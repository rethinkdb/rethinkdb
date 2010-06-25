
#ifndef __IO_CALLS_HPP__
#define __IO_CALLS_HPP__

#include "arch/resource.hpp"
#include "event.hpp"

struct event_queue_t;

struct io_calls_t {
    /**
     * Synchronous (but possibly non-blocking, depending on fd) API
     */
    static ssize_t read(resource_t fd, void *buf, size_t count);
    static ssize_t write(resource_t fd, const void *buf, size_t count);

    /**
     * Asynchronous API
     */
    // Submit an asynchronous read request to the OS
    void schedule_aio_read(resource_t resource,
                           size_t offset, size_t length, void *buf,
                           event_queue_t *notify_target, void *state);

    // Submit asynchronous write requests to the OS
    struct aio_write_t {
    public:
        resource_t      resource;
        size_t          offset;
        size_t          length;
        void            *buf;
        void            *state;
    };
    void schedule_aio_write(aio_write_t *writes, int num_writes, event_queue_t *notify_target);
};

#endif // __IO_CALLS_HPP__

