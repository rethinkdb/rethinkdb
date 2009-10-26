
#include "utils.hpp"
#include "async_io.hpp"

void schedule_aio_read(resource_t resource,
                       size_t offset, size_t length, void *buf,
                       event_queue_t *notify_target, void *state,
                       alloc_blackhole_t *allocator)
{
    iocb *request = (iocb*)malloc(allocator, sizeof(iocb));
    io_prep_pread(request, resource, buf, length, offset);
    request->data = state;
    iocb* requests[1];
    requests[0] = request;
    int res = io_submit(notify_target->aio_context, 1, requests);
    check("Could not submit IO request", res < 1);
}
