
#ifndef __ARCH_LINUX_DISK_AIO_GETEVENTS_EVENTFD_HPP__
#define __ARCH_LINUX_DISK_AIO_GETEVENTS_EVENTFD_HPP__

#include "arch/linux/disk/aio.hpp"

/* This strategy for calling io_getevents() uses io_set_eventfd(), which is only available on
some systems. */

struct linux_aio_getevents_eventfd_t :
    public linux_aio_getevents_t, public linux_event_callback_t
{
    explicit linux_aio_getevents_eventfd_t(linux_diskmgr_aio_t *parent);
    ~linux_aio_getevents_eventfd_t();

    fd_t aio_notify_fd;

    linux_diskmgr_aio_t *parent;

    void prep(iocb *req);
    void on_event(int events);
};

#endif // __ARCH_LINUX_DISK_AIO_GETEVENTS_EVENTFD_HPP__

