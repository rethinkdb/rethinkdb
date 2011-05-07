#ifndef __ARCH_LINUX_DISK_AIO_SUBMIT_SYNC_HPP__
#define __ARCH_LINUX_DISK_AIO_SUBMIT_SYNC_HPP__

#include "arch/linux/disk/aio.hpp"
#include <vector>

struct linux_aio_submit_sync_t :
    public linux_diskmgr_aio_t::submit_strategy_t,
    public watchable_t<bool>::watcher_t
{
    linux_aio_submit_sync_t(linux_aio_context_t *context, passive_producer_t<iocb *> *source);
    ~linux_aio_submit_sync_t();
    void notify_done();

private:
    void on_watchable_changed();
    void pump();
    linux_aio_context_t *context;
    passive_producer_t<iocb *> *source;
    int n_pending;
    std::vector<iocb *> request_batch;
};

#endif /* __ARCH_LINUX_DISK_AIO_SUBMIT_SYNC_HPP__ */
