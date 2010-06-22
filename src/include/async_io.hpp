
#ifndef __ASYNC_IO_HPP__
#define __ASYNC_IO_HPP__

#include "event.hpp"

struct event_queue_t;

// TODO: implement scatter/gather IO.

// Sending a single IO read request to a file
void schedule_aio_read(resource_t resource,
                       size_t offset, size_t length, void *buf,
                       event_queue_t *notify_target, void *state);

struct aio_write_t {
    public:
        resource_t      resource;
        size_t          offset;
        size_t          length;
        void            *buf;
        void            *state;
};

//Sending multiple IO write requests to a file
void schedule_aio_write(aio_write_t *writes, int num_writes, event_queue_t *notify_target);


#endif // __ASYNC_IO_HPP__

