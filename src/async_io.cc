
#include "utils.hpp"
#include "async_io.hpp"

void schedule_aio_read(resource_t resource,
                       size_t offset, size_t length, char *buf,
                       event_queue_t *notify_target,
                       alloc_blackhole_t *allocator)
{
    size_t old_alignment = get_alignment(allocator);
    set_alignment(allocator, 512);
    iocb *request = (iocb*)malloc(allocator, sizeof(iocb));
    set_alignment(allocator, old_alignment);
    io_prep_pread(request, resource, buf, length, offset);
    iocb* requests[1];
    requests[0] = request;
    int res = io_submit(notify_target->aio_context, 1, requests);
    check("Could not submit IO request", res < 1);
}
