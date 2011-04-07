#include "arch/linux/disk/aio/submit_threaded.hpp"
#include "logger.hpp"
#include <sched.h>   // For sched_yield()

linux_aio_submit_threaded_t::linux_aio_submit_threaded_t(linux_diskmgr_aio_t *parent)
    : parent(parent), blocker_pool(1, parent->queue), n_pending(0), active(false)
{
}

linux_aio_submit_threaded_t::~linux_aio_submit_threaded_t() {

    rassert(n_pending == 0);
    rassert(accum_queue.empty());
    rassert(!active);

    rassert(perform_queue.empty());
}

/* There are three conditions that must be true for us to start a new round of io_submit()ting:

1. The number of active IO requests ('n_pending') must be less than TARGET_IO_QUEUE_DEPTH.
2. There must be requests waiting to run in accum_queue.
3. We must not currently have an active round ('active' must be false)

When any of these conditions becomes true, we call maybe_start_round() to check the other two
conditions and then start a new round if appropriate. */

void linux_aio_submit_threaded_t::notify_done(iocb *) {
    rassert(n_pending > 0);
    //dependencies.unregister_request(event);
    n_pending--;
    maybe_start_round();
}

void linux_aio_submit_threaded_t::submit(iocb *request) {
    accum_queue.push(request);
    maybe_start_round();
}

void linux_aio_submit_threaded_t::done() {
    rassert(active);
    active = false;
    maybe_start_round();
}

void linux_aio_submit_threaded_t::maybe_start_round() {

    if (n_pending == TARGET_IO_QUEUE_DEPTH) return;
    if (accum_queue.empty()) return;
    if (active) return;

    rassert(perform_queue.empty());

    /* Transfer the iocbs to send into the perform queue */
    size_t target_batch_size = TARGET_IO_QUEUE_DEPTH - n_pending;
    perform_queue.reserve(target_batch_size);
    while (!accum_queue.empty() && perform_queue.size() < target_batch_size) {
        //if (dependencies.is_conflicting(accum_queue.peek()))
        //    break;

        iocb *request = accum_queue.pull();
        //dependencies.register_active_request(request);
        perform_queue.push_back(request);
        ++n_pending;
    }

    active = true;

    /* Trigger the job to start */
    blocker_pool.do_job(this);
}

void linux_aio_submit_threaded_t::run() {

    /* Called in another thread */
    rassert(active);
    rassert(!perform_queue.empty());

    int pos = 0;
    while (pos < (int)perform_queue.size()) {
        int res = io_submit(parent->aio_context.id, perform_queue.size()-pos, perform_queue.data()+pos);
        if (res == -EAGAIN) {
            /* Give up our CPU time slot and give some time for IO operations to be done */
            int res2 = sched_yield();
            guarantee_err(res2 == 0, "sched_yield() failed");   // Never happens on Linux
        } else if (res < 0) {
            crash("io_submit() failed: (%d) %s\n", -res, strerror(-res));
        } else {
            pos += res;
        }
    }

    perform_queue.clear();
}

