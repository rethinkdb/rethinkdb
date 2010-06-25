
#include <unistd.h>
#include <stdio.h>
#include "arch/io.hpp"
#include "utils.hpp"
#include "event_queue.hpp"
#include "alloc/memalign.hpp"
#include "alloc/pool.hpp"
#include "alloc/dynamic_pool.hpp"
#include "alloc/stats.hpp"
#include "alloc/alloc_mixin.hpp"

ssize_t io_calls_t::read(resource_t fd, void *buf, size_t count) {
    return ::read(fd, buf, count);
}

ssize_t io_calls_t::write(resource_t fd, const void *buf, size_t count) {
    return ::write(fd, buf, count);
}

void io_calls_t::schedule_aio_read(resource_t resource,
                                   size_t offset, size_t length, void *buf,
                                   event_queue_t *notify_target, iocallback_t *callback)
{
    // Prepare the request
    iocb *request = new iocb();
    io_prep_pread(request, resource, buf, length, offset);
    io_set_eventfd(request, notify_target->aio_notify_fd);
    request->data = callback;

    // Add it to a list of outstanding requests
    remaining_requests.push_back(request);

    // Process whatever is left
    process_remaining_requests();
}

// TODO: we should build a flow control/diagnostics system into the
// AIO scheduler. If the system can't handle too much IO, we should
// slow down responses to the client in case they're using our API
// synchroniously, and also stop reading from sockets so their socket
// buffers fill up during sends in case they're using our API
// asynchronously.
void io_calls_t::schedule_aio_write(aio_write_t *writes, int num_writes, event_queue_t *notify_target) {
    // TODO: watch how we're allocating
    int i;
    for (i = 0; i < num_writes; i++) {
        // Prepare the request
        iocb *request = new iocb();
        io_prep_pwrite(request, writes[i].resource, writes[i].buf, writes[i].length, writes[i].offset);
        io_set_eventfd(request, notify_target->aio_notify_fd);
        request->data = writes[i].callback;

        // Add it to a list of outstanding requests
        remaining_requests.push_back(request);
    }

    // Process whatever is left
    process_remaining_requests();
}

void io_calls_t::aio_notify(event_t *event) {
    // Schedule the requests we couldn't finish last time
    process_remaining_requests();
    
    // Notify the interested party about the event
    iocallback_t *callback = (iocallback_t*)event->state;
    event->state = NULL;
    callback->on_io_complete(event);
}

void io_calls_t::process_remaining_requests() {
    int res = 0;
    while(!remaining_requests.empty() && res >= 0) {
        res = io_submit(queue->aio_context,
                        remaining_requests.size(),
                        &remaining_requests[0]);
        if(res > 0)
            remaining_requests.erase(remaining_requests.begin(), remaining_requests.begin() + res);
    }
    check("Could not submit IO write request",
          res < 0 && res != -EAGAIN);
}
