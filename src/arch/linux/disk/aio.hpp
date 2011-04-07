
#ifndef __ARCH_LINUX_DISK_AIO_COMMON_HPP__
#define __ARCH_LINUX_DISK_AIO_COMMON_HPP__

#include <libaio.h>
#include "errors.hpp"
#include <boost/scoped_ptr.hpp>
#include "arch/linux/system_event.hpp"
#include "utils2.hpp"
#include "config/args.hpp"
#include "arch/linux/event_queue.hpp"
#include "arch/linux/disk/diskmgr.hpp"

/* Disk manager that uses libaio. We have two strategies for calling io_submit() and two
strategies for calling io_getevents() depending on the platform.

TODO: linux_diskmgr_aio_t should only be used for IO strategies that use libaio and io_context_t.
However, as a hack we are temporarily implementing thread-pool-based IO as an io_submit()
strategy instead of as a separate class of disk manager. The motive for this hack was to make it
easier to share reordering/locking logic between the libaio-based and pool-based disk managers,
because for now the reordering/locking logic is implemented in terms of iocb* objects. This is bad
and we should get rid of it later. */

struct linux_aio_submit_t {
    virtual ~linux_aio_submit_t() { }

    /* Called to submit an IO operation to the OS */
    virtual void submit(iocb *) = 0;

    /* Called to inform the submitter that the OS has completed an IO operation, so it can e.g.
    try to send more events to the OS. */
    virtual void notify_done(iocb *) = 0;
};

struct linux_aio_getevents_t {

    /* The linux_aio_getevents_t subclass will be given a pointer to the linux_diskmgr_aio_t when
    it is created, and it must call aio_notify() whenever it gets some events. */

    virtual ~linux_aio_getevents_t() { }

    /* Will be called on every IO request before it's sent to the OS. Some subclasses of
    linux_aio_getevents_t use it to call io_set_eventfd(). */
    virtual void prep(iocb *) = 0;
};

/* Simple wrapper around io_context_t that handles creation and destruction */

struct linux_aio_context_t {
    linux_aio_context_t(int max_concurrent);
    ~linux_aio_context_t();
    io_context_t id;
};

/* IO requests are sent to the linux_diskmgr_aio_t via pwrite() and pread(). It constructs iocbs
for them, calls linux_aio_getevents_t::prep() on them, and then calls linux_aio_submit_t::submit().
When the OS is done, the linux_aio_getevents_t finds out and calls linux_diskmgr_aio_t::aio_notify(),
which calls linux_aio_submit_t::notify_done() as well as the callback that was passed to
pwrite()/pread(). */

class linux_diskmgr_aio_t : public linux_diskmgr_t {

public:
    explicit linux_diskmgr_aio_t(linux_event_queue_t *queue, const linux_io_backend_t io_backend);
    virtual ~linux_diskmgr_aio_t();
    void pread(fd_t fd, void *buf, size_t count, off_t offset, linux_disk_op_t *cb);
    void pwrite(fd_t fd, const void *buf, size_t count, off_t offset, linux_disk_op_t *cb);

public:
    /* Internal stuff which is public so that our linux_aio_submit_t and linux_aio_getevents_t
    objects can access it. */

    linux_event_queue_t *queue;

    /* This must be declared before "submitter" and "getter" so that it gets created before they
    are created and destroyed after they are destroyed. */
    linux_aio_context_t aio_context;

    boost::scoped_ptr<linux_aio_submit_t> submitter;

    boost::scoped_ptr<linux_aio_getevents_t> getter;
    void aio_notify(iocb *event, int result);   // Called by getter
};

#endif // __ARCH_LINUX_DISK_AIO_COMMON_HPP__

