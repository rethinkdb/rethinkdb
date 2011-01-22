
#ifndef __ARCH_LINUX_LINUX_IO_CALLS_NO_EVENTFD_HPP__
#define __ARCH_LINUX_LINUX_IO_CALLS_NO_EVENTFD_HPP__

#include "arch/linux/disk/linux_io_calls_base.hpp"

class linux_io_calls_no_eventfd_t : public linux_io_calls_base_t {
public:
    explicit linux_io_calls_no_eventfd_t(linux_event_queue_t *queue);
    ~linux_io_calls_no_eventfd_t();

    system_event_t aio_notify_event;
    pthread_t io_thread;
    pthread_mutex_t io_mutex;
    bool shutting_down;
    
    typedef std::vector<io_event> io_event_vector_t;
    io_event_vector_t io_events;

public:
    void on_event(int events);
};

#endif // __ARCH_LINUX_LINUX_IO_CALLS_NO_EVENTFD_HPP__

