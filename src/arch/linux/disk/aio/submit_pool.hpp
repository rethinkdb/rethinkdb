#ifndef __SUBMIT_POOL_HPP__
#define	__SUBMIT_POOL_HPP__

#include "arch/linux/disk/aio.hpp"
#include "arch/linux/blocker_pool.hpp"
#include "arch/linux/disk/reordering_io_queue.hpp"
#include "arch/linux/disk/concurrent_io_dependencies.hpp"

/* The linux_aio_submit_pooled_t strategy uses a blocker thread pool and emulates kernel aio
 * by dispatching blocking io requests across the different threads in the pool */

// TODO: This should NOT be a linux_aio_submit_t because it doesn't use libaio and io_context.
// Instead it should be a separate disk manager.

struct linux_aio_submit_pooled_t :
    public linux_aio_submit_t
{
    linux_aio_submit_pooled_t(linux_diskmgr_aio_t *parent);
    ~linux_aio_submit_pooled_t();
    void submit(iocb *);
    void notify_done(iocb *event);

private:
    friend class iocb_job_t;
    
    linux_diskmgr_aio_t *parent;
    blocker_pool_t blocker_pool;

    /* n_pending is size of the set of iocbs in the blocker_pool */
    int n_pending;

    reordering_io_queue_t accum_queue;
    concurrent_io_dependencies_t dependencies;

    void maybe_start_round();
};

#endif	/* __SUBMIT_POOL_HPP__ */

