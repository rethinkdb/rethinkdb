
#ifndef __EVENT_QUEUE_HPP__
#define __EVENT_QUEUE_HPP__

#include "perfmon.hpp"
#include "linux_utils.hpp"
#include <boost/function.hpp>

// Event queue callback
struct linux_event_callback_t {
    virtual void on_event(int events) = 0;
    virtual ~linux_event_callback_t() {}
};

// Common event queue functionality
struct event_queue_base_t {
public:
    void watch_signal(const sigevent *evp, linux_event_callback_t *cb);
    void forget_signal(const sigevent *evp, linux_event_callback_t *cb);

private:
    static void signal_handler(int signum, siginfo_t *siginfo, void *uctx);
};

struct linux_queue_parent_t {
    virtual void pump() = 0;
    virtual bool should_shut_down() = 0;
    virtual ~linux_queue_parent_t() {}
};

const int poll_event_in = 1;
const int poll_event_out = 2;
const int poll_event_err = 4;
const int poll_event_hup = 8;
const int poll_event_rdhup = 16;

std::string format_poll_event(int event);

// Queue stats (declared here so whichever queue is chosen can access it)
extern perfmon_duration_sampler_t pm_eventloop;

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

/* linux_event_watcher_t can be used to wait for read and write events in the
event queue. Construct one with an FD. Then call wait() to wait for events.

If the event loop produces a poll_event_{err,hup,rdhup}, then the provided
error handler will be called. The provided error handler must cancel any
outstanding 

You can wait for read and write events concurrently, but not on two separate
threads. */

struct linux_event_watcher_guts_t;   // Forward declared due to circular dependency with signal_t
struct signal_t;

struct linux_event_watcher_t {

    linux_event_watcher_t(fd_t, linux_event_callback_t *error_handler);
    ~linux_event_watcher_t();

    /* watch()'s first parameter should be poll_event_in or poll_event_out. The
    second parameter is a function to call when the desired event is received. The
    third parameter is a signal_t; if the signal_t is pulsed, the watch will be
    cancelled. */
    void watch(int event, const boost::function<void()> &callback, signal_t *aborter);

private:
    /* The guts are a separate object so that if one of the callbacks we call destroys us,
    we don't have to destroy the guts immediately. */
    linux_event_watcher_guts_t *guts;
    DISABLE_COPYING(linux_event_watcher_t);
};

#endif // __EVENT_QUEUE_HPP__
