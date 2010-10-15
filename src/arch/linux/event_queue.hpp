
#ifndef __EVENT_QUEUE_HPP__
#define __EVENT_QUEUE_HPP__

// Event queue callback
struct linux_event_callback_t {
    virtual void on_event(int events) = 0;
};

// Some other stuff
typedef int fd_t;
#define INVALID_FD fd_t(-1)

struct linux_queue_parent_t {
    virtual void pump() = 0;
    virtual bool should_shut_down() = 0;
};

/* Pick the queue now*/
#ifdef NO_EPOLL

// Use poll instead of epoll
#include "arch/linux/event_queue/poll.hpp"
typedef poll_event_queue_t linux_event_queue_t;

#else

// Epoll to the rescue
#include "arch/linux/event_queue/epoll.hpp"
typedef epoll_event_queue_t linux_event_queue_t;

#endif

#endif // __EVENT_QUEUE_HPP__
