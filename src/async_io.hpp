
#ifndef __ASYNC_IO_HPP__
#define __ASYNC_IO_HPP__

#include "event_queue.hpp"

void schedule_aio_read(resource_t resource,
                       size_t offset, size_t length, char *buf,
                       event_queue_t *notify_target);

#endif // __ASYNC_IO_HPP__

