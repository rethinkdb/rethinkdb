
#ifndef __EPOLL_EVENT_QUEUE_HPP__
#define __EPOLL_EVENT_QUEUE_HPP__

#include <queue>
#include <sys/epoll.h>
#include "config/args.hpp"
#include "perfmon.hpp"

// Event queue structure
struct epoll_event_queue_t {
public:
    explicit epoll_event_queue_t(linux_queue_parent_t *parent);
    void run();
    ~epoll_event_queue_t();

private:
    linux_queue_parent_t *parent;
    
    fd_t epoll_fd;
    
    // We store this as a class member because forget_resource needs
    // to go through the events and remove queued messages for
    // resources that are being destroyed.
    epoll_event events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];
    int nevents;

public:
    // These should only be called by the event queue itself or by the linux_* classes
    void watch_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void adjust_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void forget_resource(fd_t resource, linux_event_callback_t *cb);
};


#endif // __EPOLL_EVENT_QUEUE_HPP__
