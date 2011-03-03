#include "arch/linux/disk/aio/submit_pool.hpp"
#include "logger.hpp"

struct iocb_job_t : public blocker_pool_t::job_t {
    iocb *io;
    linux_aio_submit_pooled_t *parent;

    iocb_job_t(iocb *io, linux_aio_submit_pooled_t *parent)
        : io(io), parent(parent) { }
    void run() {
        fd_t fd = io->aio_fildes;
        void *buf = io->u.c.buf;
        size_t count = io->u.c.nbytes;
        off64_t offset = io->u.c.offset;

        if (io->aio_lio_opcode == IO_CMD_PREAD) {
            int res = pread(fd, buf, count, offset);
            guarantee_err(res == (ssize_t)count, "pread() failed");
        }
        else {
            rassert(io->aio_lio_opcode == IO_CMD_PWRITE);
            int res = pwrite(fd, buf, count, offset);
            guarantee_err(res == (ssize_t)count, "pread() failed");
        }
    }
    void done() {
        parent->parent->aio_notify(io, (int)io->u.c.nbytes);
        delete this;
    }
};



linux_aio_submit_pooled_t::linux_aio_submit_pooled_t(linux_diskmgr_aio_t *parent)
    : parent(parent), blocker_pool(MAX_CONCURRENT_IO_REQUESTS, parent->queue), n_pending(0)
{
}

linux_aio_submit_pooled_t::~linux_aio_submit_pooled_t() {

    rassert(n_pending == 0);
    rassert(accum_queue.empty());
}

void linux_aio_submit_pooled_t::notify_done(iocb *event) {
    rassert(n_pending > 0);
    dependencies.unregister_request(event);
    n_pending--;
    maybe_start_round();
}

void linux_aio_submit_pooled_t::submit(iocb *request) {
    accum_queue.push(request);
    maybe_start_round();
}

void linux_aio_submit_pooled_t::maybe_start_round() {
    /* Issue iocbs to the block pool */
    while (!accum_queue.empty() && n_pending < TARGET_IO_QUEUE_DEPTH) {
        if (dependencies.is_conflicting(accum_queue.peek()))
            break;

        iocb *request = accum_queue.pull();
        dependencies.register_active_request(request);
        blocker_pool.do_job(new iocb_job_t(request, this));
        ++n_pending;
    }
}


