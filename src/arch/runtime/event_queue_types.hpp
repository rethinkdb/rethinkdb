// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_EVENT_QUEUE_TYPES_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_TYPES_HPP_

#include <signal.h>

// Types that are used, in particular, by poll.hpp and epoll.hpp.

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


#endif  // ARCH_RUNTIME_EVENT_QUEUE_TYPES_HPP_

