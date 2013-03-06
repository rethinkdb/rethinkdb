// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifdef AIOSUPPORT

#include "arch/io/disk/aio/submit_sync.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

linux_aio_submit_sync_t::linux_aio_submit_sync_t(
        linux_aio_context_t *_context, passive_producer_t<iocb *> *_source) :
    context(_context), source(_source), n_pending(0)
{
    /* If there are already operations waiting to go, start processing them. */
    if (source->available->get()) pump();

    /* Register so we get notified when requests come in */
    source->available->set_callback(this);
}

linux_aio_submit_sync_t::~linux_aio_submit_sync_t() {
    rassert(n_pending == 0);
    source->available->unset_callback();
}

void linux_aio_submit_sync_t::notify_done() {
    /* notify_done() is called by the linux_diskmgr_aio_t when an operation is completed.
    We use it to keep the OS's IO queue always at a certain depth. */

    n_pending--;
    pump();
}

void linux_aio_submit_sync_t::on_source_availability_changed() {
    /* This is called when data becomes available on the source-queue or when
    it stops being available. If it became available, we need to start the cycle
    of pulling operations off the source queue and running them. */
    if (source->available->get()) pump();
}

void linux_aio_submit_sync_t::pump() {

    while (source->available->get() && n_pending < TARGET_IO_QUEUE_DEPTH) {

        // Fill up the batch
        size_t target_batch_size = TARGET_IO_QUEUE_DEPTH - n_pending;
        request_batch.reserve(target_batch_size);
        while (source->available->get() && request_batch.size() < target_batch_size) {
            request_batch.push_back(source->pop());
        }

        if (request_batch.empty())
            break;

        int actual_size = io_submit(context->id, request_batch.size(), request_batch.data());

        if (actual_size == -EAGAIN) {
            break;
        } else if (actual_size < 0) {
            crash("io_submit() failed: (%d) %s\n", -actual_size, errno_string(-actual_size).c_str());
        } else {
            request_batch.erase(request_batch.begin(), request_batch.begin() + actual_size);
            n_pending += actual_size;
        }
    }
}
#endif // AIOSUPPORT
