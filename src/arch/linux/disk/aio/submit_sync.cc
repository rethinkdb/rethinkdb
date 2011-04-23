#include <vector>

#include "arch/linux/disk/aio/submit_sync.hpp"
#include "logger.hpp"

linux_aio_submit_sync_t::linux_aio_submit_sync_t(linux_diskmgr_aio_t *parent) :
    parent(parent), n_pending(0) { }

linux_aio_submit_sync_t::~linux_aio_submit_sync_t() {
    rassert(n_pending == 0);
    rassert(queue.empty());
}

void linux_aio_submit_sync_t::submit(iocb *operation) {
    queue.push_back(operation);
    pump();
}

void linux_aio_submit_sync_t::notify_done() {
    n_pending--;
    pump();
}

void linux_aio_submit_sync_t::pump() {
    while (!queue.empty() && n_pending < TARGET_IO_QUEUE_DEPTH) {

        // Fill up the batch
        size_t target_batch_size = TARGET_IO_QUEUE_DEPTH - n_pending;
        request_batch.reserve(target_batch_size);
        while (!queue.empty() && request_batch.size() < target_batch_size) {
            request_batch.push_back(queue.front());
            queue.pop_front();
        }

        if (request_batch.size() == 0)
            break;

        int actual_size = io_submit(parent->aio_context.id, request_batch.size(), request_batch.data());

        if (actual_size == -EAGAIN) {
            break;
        } else if (actual_size < 0) {
            crash("io_submit() failed: (%d) %s\n", -actual_size, strerror(-actual_size));
        } else {
            request_batch.erase(request_batch.begin(), request_batch.begin() + actual_size);
            n_pending += actual_size;
        }
    }
}
