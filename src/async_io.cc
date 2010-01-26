
#include "utils.hpp"
#include "async_io.hpp"
#include "event_queue.hpp"

void schedule_aio_read(resource_t resource,
                       size_t offset, size_t length, void *buf,
                       event_queue_t *notify_target, event_state_t *state)
{
    iocb *request = notify_target->alloc.malloc<iocb>();
    io_prep_pread(request, resource, buf, length, offset);
    io_set_eventfd(request, notify_target->aio_notify_fd);
    request->data = state;
    iocb* requests[1];
    requests[0] = request;
    int res = io_submit(notify_target->aio_context, 1, requests);
    check("Could not submit IO read request", res < 1);
}

void schedule_aio_write(resource_t resource,
                        size_t offset, size_t length, void *buf,
                        event_queue_t *notify_target, event_state_t *state)
{
    iocb *request = notify_target->alloc.malloc<iocb>();
    io_prep_pwrite(request, resource, buf, length, offset);
    io_set_eventfd(request, notify_target->aio_notify_fd);
    request->data = state;
    iocb* requests[1];
    requests[0] = request;
    int res = io_submit(notify_target->aio_context, 1, requests);
    check("Could not submit IO write request", res < 1);
}

