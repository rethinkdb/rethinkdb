#ifndef __ARCH_LINUX_DISK_AIO_SUBMIT_SYNC_HPP__
#define __ARCH_LINUX_DISK_AIO_SUBMIT_SYNC_HPP__

#include "arch/linux/disk/aio.hpp"
#include <list>
#include <vector>

struct linux_aio_submit_sync_t : public linux_diskmgr_aio_t::submit_strategy_t {

    linux_aio_submit_sync_t(linux_diskmgr_aio_t *parent);
    ~linux_aio_submit_sync_t();
    void submit(iocb *);
    void notify_done();

private:
    void pump();
    linux_diskmgr_aio_t *parent;
    int n_pending;
    std::list<iocb *> queue;
    std::vector<iocb *> request_batch;
};

#endif /* __ARCH_LINUX_DISK_AIO_SUBMIT_SYNC_HPP__ */
