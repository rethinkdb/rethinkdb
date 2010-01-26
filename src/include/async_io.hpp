
#ifndef __ASYNC_IO_HPP__
#define __ASYNC_IO_HPP__

#include "event.hpp"

struct event_queue_t;

// Sending a single IO read request to a file
void schedule_aio_read(resource_t resource,
                       size_t offset, size_t length, void *buf,
                       event_queue_t *notify_target, event_state_t *state);

// Sending a single IO write request to a file
void schedule_aio_write(resource_t resource,
                        size_t offset, size_t length, void *buf,
                        event_queue_t *notify_target, event_state_t *state);

#endif // __ASYNC_IO_HPP__

