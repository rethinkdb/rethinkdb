
#ifndef __EVENT_QUEUE_HPP__
#define __EVENT_QUEUE_HPP__

#include "perfmon.hpp"

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

const int poll_event_in = 1;
const int poll_event_out = 2;
const int poll_event_err = 4;

// Queue stats (declared here so whichever queue is chosen can access it)
extern perfmon_thread_average_t pm_events_per_loop;

/* Pick the queue now*/
//#define NO_EPOLL

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
