#ifndef ARCH_IO_DISK_AIO_SUBMIT_SYNC_HPP_
#define ARCH_IO_DISK_AIO_SUBMIT_SYNC_HPP_

#include <vector>

#include "arch/io/disk/aio.hpp"

struct linux_aio_submit_sync_t :
    private availability_callback_t,
    public linux_diskmgr_aio_t::submit_strategy_t
{
    linux_aio_submit_sync_t(linux_aio_context_t *context, passive_producer_t<iocb *> *source);
    ~linux_aio_submit_sync_t();
    void notify_done();

private:
    void on_source_availability_changed();
    void pump();
    linux_aio_context_t *context;
    passive_producer_t<iocb *> *source;
    int n_pending;
    std::vector<iocb *> request_batch;
};

#endif /* ARCH_IO_DISK_AIO_SUBMIT_SYNC_HPP_ */
