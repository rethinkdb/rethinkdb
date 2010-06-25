
#ifndef __IO_CALLS_HPP__
#define __IO_CALLS_HPP__

#include "arch/resource.hpp"
#include "event.hpp"

struct event_queue_t;

struct iocallback_t {
    virtual void on_io_complete(event_t *event) = 0;
};

struct io_calls_t {
public:
    io_calls_t(event_queue_t *_queue)
        : queue(_queue)
        {}
    
    /**
     * Synchronous (but possibly non-blocking, depending on fd) API
     */
    static ssize_t read(resource_t fd, void *buf, size_t count);
    static ssize_t write(resource_t fd, const void *buf, size_t count);

    /**
     * Asynchronous API
     */
    // Submit an asynchronous read request to the OS
    void schedule_aio_read(resource_t resource,
                           size_t offset, size_t length, void *buf,
                           event_queue_t *notify_target, iocallback_t *callback);

    // Submit asynchronous write requests to the OS
    struct aio_write_t {
    public:
        resource_t      resource;
        size_t          offset;
        size_t          length;
        void            *buf;
        iocallback_t    *callback;
    };
    void schedule_aio_write(aio_write_t *writes, int num_writes, event_queue_t *notify_target);

    /**
     * AIO notification support (this is meant to be called by the
     * event queue).
     */
    void aio_notify(event_t *event);

private:
    event_queue_t *queue;
};

#endif // __IO_CALLS_HPP__

