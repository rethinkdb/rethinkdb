#ifndef __ARCH_LINUX_DISK_AIO_SUBMIT_SYNC_HPP__
#define __ARCH_LINUX_DISK_AIO_SUBMIT_SYNC_HPP__

#include "arch/linux/disk/aio.hpp"
#include "arch/linux/disk/reordering_io_queue.hpp"
#include "arch/linux/disk/concurrent_io_dependencies.hpp"

struct linux_aio_submit_sync_t : public linux_aio_submit_t {

    linux_aio_submit_sync_t(linux_diskmgr_aio_t *parent);
    ~linux_aio_submit_sync_t();
    void submit(iocb *);
    void notify_done(iocb *);

private:
    void pump();
    linux_diskmgr_aio_t *parent;
    int n_pending;
    reordering_io_queue_t queue;
    concurrent_io_dependencies_t dependencies;
    std::vector<iocb *> request_batch;
};

#endif /* __ARCH_LINUX_DISK_AIO_SUBMIT_SYNC_HPP__ */
