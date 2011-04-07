#include <algorithm>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "arch/linux/disk/aio.hpp"
#include "arch/linux/disk/aio/getevents_eventfd.hpp"
#include "arch/linux/disk/aio/getevents_noeventfd.hpp"
#include "arch/linux/disk/aio/submit_sync.hpp"
#include "arch/linux/disk/aio/submit_pool.hpp"
#include "arch/linux/disk/aio/submit_threaded.hpp"
#include "arch/linux/arch.hpp"
#include "config/args.hpp"
#include "utils2.hpp"
#include "logger.hpp"

linux_aio_context_t::linux_aio_context_t(int max_concurrent) {
    id = 0;
    int res = io_setup(max_concurrent, &id);
    guarantee_xerr(res == 0, -res, "Could not setup aio context");
}

linux_aio_context_t::~linux_aio_context_t() {
    int res = io_destroy(id);
    guarantee_xerr(res == 0, -res, "Could not destroy aio context");
}

/* Async IO scheduler - the common base */
linux_diskmgr_aio_t::linux_diskmgr_aio_t(linux_event_queue_t *queue, const linux_io_backend_t io_backend) :
    queue(queue),
    aio_context(MAX_CONCURRENT_IO_REQUESTS)
{
    if(io_backend == aio_native) {
#ifdef IO_SUBMIT_THREADED
        submitter.reset(new linux_aio_submit_threaded_t(this));
#else
        submitter.reset(new linux_aio_submit_sync_t(this));
#endif
    } else {
        submitter.reset(new linux_aio_submit_pooled_t(this));
    }

#ifdef NO_EVENTFD
    getter.reset(new linux_aio_getevents_noeventfd_t(this));
#else
    getter.reset(new linux_aio_getevents_eventfd_t(this));
#endif
}

linux_diskmgr_aio_t::~linux_diskmgr_aio_t() {
    // linux_aio_context_t destructor handles destroying the aio context
}

void linux_diskmgr_aio_t::pread(fd_t fd, void *buf, size_t count, off_t offset, linux_disk_op_t *cb) {

    // Prepare the request
    iocb *request = new iocb;
    io_prep_pread(request, fd, buf, count, offset);
    request->data = reinterpret_cast<void*>(cb);

    getter->prep(request);
    submitter->submit(request);
}

void linux_diskmgr_aio_t::pwrite(fd_t fd, const void *buf, size_t count, off_t offset, linux_disk_op_t *cb) {

    // Prepare the request
    iocb *request = new iocb;
    io_prep_pwrite(request, fd, const_cast<void*>(buf), count, offset);
    request->data = reinterpret_cast<void*>(cb);

    getter->prep(request);
    submitter->submit(request);
}

void linux_diskmgr_aio_t::aio_notify(iocb *event, int result) {
    // Notify the submitter in case it wants to e.g. send more operations to the OS.
    submitter->notify_done(event);

    // Check for failure.
    // TODO: Do we have a use case for passing on errors to higher-level code instead of crashing?
    // Perhaps we should retry the failed IO operation?
    guarantee_xerr(result == (int)event->u.c.nbytes, -result, "Read or write failed");
    
    // Notify the interested party about the event
    linux_disk_op_t *callback = (linux_disk_op_t*)event->data;
    callback->done();
    
    // Free the iocb structure
    delete event;
}
