
#ifndef __EVENT_QUEUE_HPP__
#define __EVENT_QUEUE_HPP__

#include <queue>
#include <sys/epoll.h>
#include "corefwd.hpp"
#include "config/args.hpp"

typedef int fd_t;
#define INVALID_FD fd_t(-1)

struct linux_epoll_callback_t {
    virtual void on_epoll(int events) = 0;
};

struct linux_queue_parent_t {
    virtual void pump() = 0;
    virtual bool should_shut_down() = 0;
};

// Event queue structure
struct linux_event_queue_t {

public:
    linux_event_queue_t(linux_queue_parent_t *parent);
    void run();
    ~linux_event_queue_t();

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
    void watch_resource(fd_t resource, int events, linux_epoll_callback_t *cb);
    void forget_resource(fd_t resource, linux_epoll_callback_t *cb);
};


#endif // __EVENT_QUEUE_HPP__
