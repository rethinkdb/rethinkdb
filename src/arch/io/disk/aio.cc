#if AIOSUPPORT

#include "arch/io/disk/aio.hpp"

#include <linux/fs.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <algorithm>

#include "arch/io/disk/aio/getevents_eventfd.hpp"
#include "arch/io/disk/aio/getevents_noeventfd.hpp"
#include "arch/io/disk/aio/submit_sync.hpp"
#include "arch/io/arch.hpp"
#include "config/args.hpp"
#include "utils.hpp"

/* linux_aio_context_t */

linux_aio_context_t::linux_aio_context_t(int max_concurrent) {
    id = 0;
    int res = io_setup(max_concurrent, &id);
    guarantee_xerr(res == 0, -res, "Could not setup aio context");
}

linux_aio_context_t::~linux_aio_context_t() {
    int res = io_destroy(id);
    guarantee_xerr(res == 0, -res, "Could not destroy aio context");
}

/* linux_diskmgr_aio_t */

linux_diskmgr_aio_t::linux_diskmgr_aio_t(
        linux_event_queue_t *_queue,
        passive_producer_t<action_t *> *_source)
    : passive_producer_t<iocb *>(_source->available),
      queue(_queue),
      source(_source),
      aio_context(MAX_CONCURRENT_IO_REQUESTS) {
    submitter.init(new linux_aio_submit_sync_t(&aio_context,
                                               static_cast<passive_producer_t<iocb *> *>(this)));

#ifdef NO_EVENTFD
    getter.init(new linux_aio_getevents_noeventfd_t(this));
#else
    getter.init(new linux_aio_getevents_eventfd_t(this));
#endif
}

iocb *linux_diskmgr_aio_t::produce_next_value() {
    /* The `submitter` calls this (via the `passive_producer_t<iocb *>` interface)
    when it is ready for the next operation. */
    action_t *next_action = source->pop();
    iocb *next_iocb = next_action;
    getter->prep(next_iocb);
    return next_iocb;
}

void linux_diskmgr_aio_t::aio_notify(iocb *event, int64_t result) {

    action_t *a = static_cast<action_t *>(event);

    // Notify the submitter in case it wants to e.g. send more operations to the OS.
    submitter->notify_done();

    // Pass result up to higher level code that can decided what to do in case of failure
    a->io_result = result;

    // Pass the notification on up
    done_fun(a);
}
#endif // AIOSUPPORT
