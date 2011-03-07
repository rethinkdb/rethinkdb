#ifndef __ARCH_LINUX_DISK_AIO_SUBMIT_THREADED_HPP__
#define __ARCH_LINUX_DISK_AIO_SUBMIT_THREADED_HPP__

#include "arch/linux/disk/aio.hpp"
#include "arch/linux/blocker_pool.hpp"
#include "arch/linux/disk/reordering_io_queue.hpp"
#include "arch/linux/disk/concurrent_io_dependencies.hpp"

/* The linux_aio_submit_threaded_t strategy calls io_submit() in a separate thread. It's useful for
filesystems in which io_submit() blocks. */

struct linux_aio_submit_threaded_t :
    public linux_aio_submit_t,
    public blocker_pool_t::job_t
{
    linux_aio_submit_threaded_t(linux_diskmgr_aio_t *parent);
    ~linux_aio_submit_threaded_t();
    void submit(iocb *);
    void notify_done(iocb *event);

private:
    linux_diskmgr_aio_t *parent;
    blocker_pool_t blocker_pool;   // Has only one thread in it

    /* n_pending is size of the union of the set of iocbs in the perform queue and the set of iocbs
    in the operating system */
    int n_pending;

    bool active;   // True if we have a running job right now
    reordering_io_queue_t accum_queue;
    std::vector<iocb *> perform_queue;
    //concurrent_io_dependencies_t dependencies;

    void maybe_start_round();
    void run();   // Blocker pool calls this in a separate thread
    void done();   // Blocker pool calls this in main thread
};

#endif /* __ARCH_LINUX_DISK_AIO_SUBMIT_THREADED_HPP__ */
