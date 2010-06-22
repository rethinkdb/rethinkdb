
#include "utils.hpp"
#include "async_io.hpp"
#include "event_queue.hpp"
#include "alloc/memalign.hpp"
#include "alloc/pool.hpp"
#include "alloc/dynamic_pool.hpp"
#include "alloc/stats.hpp"
#include "alloc/alloc_mixin.hpp"

void schedule_aio_read(resource_t resource,
                       size_t offset, size_t length, void *buf,
                       event_queue_t *notify_target, void *state)
{
    iocb *request = new iocb();
    io_prep_pread(request, resource, buf, length, offset);
    io_set_eventfd(request, notify_target->aio_notify_fd);
    request->data = state;
    iocb* requests[1];
    requests[0] = request;
    int res = io_submit(notify_target->aio_context, 1, requests);
    check("Could not submit IO read request", res < 1);
}

void schedule_aio_write(aio_write_t *writes, int num_writes, event_queue_t *notify_target) {
    iocb* requests[num_writes];
    int i;
    for (i = 0; i < num_writes; i++) {
        iocb *request = new iocb();
        io_prep_pwrite(request, writes[i].resource, writes[i].buf, writes[i].length, writes[i].offset);
        io_set_eventfd(request, notify_target->aio_notify_fd);
        request->data = writes[i].state;
        requests[i] = request;
    }

    int res = io_submit(notify_target->aio_context, num_writes, requests);
    check("Could not submit IO write request", res < 1);
}
