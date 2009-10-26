
#ifndef __ASYNC_IO_HPP__
#define __ASYNC_IO_HPP__

#include "event_queue.hpp"
#include "alloc_blackhole.hpp"

// Sending a single IO read request to a file
void schedule_aio_read(resource_t resource,
                       size_t offset, size_t length, void *buf,
                       event_queue_t *notify_target,
                       alloc_blackhole_t *allocator);

#endif // __ASYNC_IO_HPP__

