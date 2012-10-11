#ifndef ARCH_IO_DISK_AIO_GETEVENTS_NOEVENTFD_HPP_
#define ARCH_IO_DISK_AIO_GETEVENTS_NOEVENTFD_HPP_
#if AIOSUPPORT

#include <vector>

#include "arch/io/disk/aio.hpp"

/* Strategy for calling io_getevents() that works when io_set_eventfd() is missing */

struct linux_aio_getevents_noeventfd_t :
    public linux_diskmgr_aio_t::getevents_strategy_t, public linux_event_callback_t
{
    explicit linux_aio_getevents_noeventfd_t(linux_diskmgr_aio_t *parent);
    ~linux_aio_getevents_noeventfd_t();

    linux_diskmgr_aio_t *parent;
    system_event_t aio_notify_event;
    pthread_t io_thread;
    pthread_mutex_t io_mutex;
    bool shutting_down;

    typedef std::vector<io_event> io_event_vector_t;
    io_event_vector_t io_events;

    static void *io_event_loop(void*);
    void prep(iocb *req);
    void on_event(int events);
};

#endif // AIOSUPPORT
#endif // ARCH_IO_DISK_AIO_GETEVENTS_NOEVENTFD_HPP_

