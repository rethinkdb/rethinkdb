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

#endif  // ARCH_RUNTIME_EVENT_QUEUE_TYPES_HPP_

