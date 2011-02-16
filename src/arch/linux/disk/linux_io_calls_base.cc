#include <algorithm>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "linux_io_calls_no_eventfd.hpp"
#include "arch/linux/arch.hpp"
#include "config/args.hpp"
#include "utils2.hpp"
#include "logger.hpp"

perfmon_counter_t
    pm_io_reads_completed("io_reads_completed[ioreads]"),
    pm_io_writes_completed("io_writes_completed[iowrites]");

/* Async IO scheduler - the common base */
linux_io_calls_base_t::linux_io_calls_base_t(linux_event_queue_t *queue)
    : queue(queue), n_pending(0), io_requests(this)
{
    int res;
    
    // Create aio context
    aio_context = 0;
    res = io_setup(MAX_CONCURRENT_IO_REQUESTS, &aio_context);
    guarantee_xerr(res == 0, -res, "Could not setup aio context");
}

linux_io_calls_base_t::~linux_io_calls_base_t()
{
    rassert(n_pending == 0);

    // Destroy the AIO context
    int res = io_destroy(aio_context);
    guarantee_xerr(res == 0, -res, "Could not destroy aio context");
}

void linux_io_calls_base_t::aio_notify(iocb *event, int result) {
    // Update stats
    if (event->aio_lio_opcode == IO_CMD_PREAD) pm_io_reads_completed++;
    else pm_io_writes_completed++;
    
    // Schedule the requests we couldn't finish last time
    n_pending--;
    process_requests();

    // Check for failure (because the higher-level code usually doesn't)
    if (result != (int)event->u.c.nbytes) {
        // Currently AIO is used only for disk files, not sockets.
        // Thus, if something fails, we have a good reason to crash
        // (note that that is not true for sockets: we should just
        // close the socket and cleanup then).
        guarantee_xerr(false, -result, "Read or write failed");
    }
    
    // Notify the interested party about the event
    linux_iocallback_t *callback = (linux_iocallback_t*)event->data;
    
    callback->on_io_complete();
    
    // Free the iocb structure
    delete event;
}

void linux_io_calls_base_t::process_requests() {
    if (n_pending > TARGET_IO_QUEUE_DEPTH)
        return;
    
    int res = 0;
    while(!io_requests.queue.empty()) {
        res = io_requests.process_request_batch();
        if(res < 0)
            break;
    }
    guarantee_xerr(res >= 0 || res == -EAGAIN, -res, "Could not submit IO request");
}

linux_io_calls_base_t::queue_t::queue_t(linux_io_calls_base_t *parent)
    : parent(parent)
{
    queue.reserve(MAX_CONCURRENT_IO_REQUESTS);
}

int linux_io_calls_base_t::queue_t::process_request_batch() {
    if (parent->n_pending > TARGET_IO_QUEUE_DEPTH)
        return -EAGAIN;
    // Submit a batch
    int res = 0;
    if(queue.size() > 0) {
        res = io_submit(parent->aio_context,
                        std::min(queue.size(), size_t(TARGET_IO_QUEUE_DEPTH / 2)),
                        &queue[0]);
        if (res > 0) {
            // TODO: erase will cause the vector to shift elements in
            // the back. Perhaps we should optimize this somehow.
            queue.erase(queue.begin(), queue.begin() + res);
            parent->n_pending += res;
        } else if (res < 0 && (-res) != EAGAIN) {
            logERR("io_submit failed: (%d) %s\n", -res, strerror(-res));
        }
    }
    return res;
}

linux_io_calls_base_t::queue_t::~queue_t() {
    rassert(queue.size() == 0);
}
