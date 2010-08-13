
#include <algorithm>
#include <unistd.h>
#include <stdio.h>
#include "arch/io.hpp"
#include "config/args.hpp"
#include "config/code.hpp"
#include "utils.hpp"
#include "event_queue.hpp"
#include "alloc/memalign.hpp"
#include "alloc/pool.hpp"
#include "alloc/dynamic_pool.hpp"
#include "alloc/stats.hpp"
#include "alloc/alloc_mixin.hpp"

io_calls_t::io_calls_t(event_queue_t *_queue)
    : queue(_queue),
      n_pending(0)
{
    r_requests.reserve(MAX_CONCURRENT_IO_REQUESTS);
    w_requests.reserve(MAX_CONCURRENT_IO_REQUESTS);
}

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
    
    assert((intptr_t)buf % DEVICE_BLOCK_SIZE == 0);
    assert(offset % DEVICE_BLOCK_SIZE == 0);
    assert(length % DEVICE_BLOCK_SIZE == 0);
    
    // Prepare the request
    iocb *request = (iocb*)tls_small_obj_alloc_accessor<alloc_t>::get_alloc<iocb>()->malloc(iocb_size);
    io_prep_pread(request, resource, buf, length, offset);
    io_set_eventfd(request, notify_target->aio_notify_fd);
    request->data = callback;

    // Add it to a list of outstanding read requests
    r_requests.push_back(request);

    // Process whatever is left
    process_requests();
}

// TODO: we should build a flow control/diagnostics system into the
// AIO scheduler. If the system can't handle too much IO, we should
// slow down responses to the client in case they're using our API
// synchroniously, and also stop reading from sockets so their socket
// buffers fill up during sends in case they're using our API
// asynchronously.
void io_calls_t::schedule_aio_write(resource_t resource,
                                    size_t offset, size_t length, void *buf,
                                    event_queue_t *notify_target, iocallback_t *callback) {
    
    assert((intptr_t)buf % DEVICE_BLOCK_SIZE == 0);
    assert(offset % DEVICE_BLOCK_SIZE == 0);
    assert(length % DEVICE_BLOCK_SIZE == 0);
    
    // Prepare the request
    iocb *request = (iocb*)tls_small_obj_alloc_accessor<alloc_t>::get_alloc<iocb>()->malloc(iocb_size);
    io_prep_pwrite(request, resource, buf, length, offset);
    io_set_eventfd(request, notify_target->aio_notify_fd);
    request->data = callback;

    // Add it to a list of outstanding write requests
    w_requests.push_back(request);
    
    // Process whatever is left
    process_requests();
}

void io_calls_t::aio_notify(iocb *event, int result) {
    // Schedule the requests we couldn't finish last time
    n_pending--;
    process_requests();
    
    // Check for failure (because the higher-level code usually doesn't)
    if (result != (int)event->u.c.nbytes) {
        errno = -result;
        check("Read or write failed", 1);
    }
    
    // Notify the interested party about the event
    iocallback_t *callback = (iocallback_t*)event->data;
    
    // Prepare event_t for the callback
    event_t qevent;
    bzero((void*)&qevent, sizeof(qevent));
    qevent.state = NULL;
    qevent.result = result;
    qevent.buf = event->u.c.buf;
    qevent.offset = event->u.c.offset;
    qevent.op = event->aio_lio_opcode == IO_CMD_PREAD ? eo_read : eo_write;
    
    callback->on_io_complete(&qevent);

    // Free the iocb structure
    tls_small_obj_alloc_accessor<alloc_t>::get_alloc<iocb>()->free(event);
}

void io_calls_t::process_requests() {
    if(n_pending > TARGET_IO_QUEUE_DEPTH)
        return;
    
    int res = 0;
    while(!r_requests.empty() || !w_requests.empty()) {
        res = process_request_batch(&r_requests);
        if(res < 0)
            break;
        
        res = process_request_batch(&w_requests);
        if(res < 0)
            break;
    }
    check("Could not submit IO request", res < 0 && res != -EAGAIN);
}

int io_calls_t::process_request_batch(request_vector_t *requests) {
    // Submit a batch
    int res = 0;
    if(requests->size() > 0) {
        res = io_submit(queue->aio_context,
                        std::min(requests->size(), size_t(TARGET_IO_QUEUE_DEPTH / 2)),
                        &requests->operator[](0));
        if(res > 0) {
            // TODO: erase will cause the vector to shift elements in
            // the back. Perhaps we should optimize this somehow.
            requests->erase(requests->begin(), requests->begin() + res);
            n_pending += res;
        }
    }
    return res;
}
