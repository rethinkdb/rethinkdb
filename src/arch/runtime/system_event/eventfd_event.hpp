// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_SYSTEM_EVENT_EVENTFD_EVENT_HPP_
#define ARCH_RUNTIME_SYSTEM_EVENT_EVENTFD_EVENT_HPP_

#include <stdint.h>
#include "errors.hpp"

// An event API implemented in terms of eventfd. May not be available
// on older kernels.
struct eventfd_event_t {
public:
    eventfd_event_t();
    ~eventfd_event_t();

    // Consumes all the pings from the eventfd.
    void consume_wakey_wakeys();

    // Pings an eventfd.  You may call this from any thread (as long as you know
    // nobody will destroy the eventfd_event_t object simultaneously).
    void wakey_wakey();

    int get_notify_fd() {
        return eventfd_;
    }

private:
    int eventfd_;
    DISABLE_COPYING(eventfd_event_t);
};

#endif // ARCH_RUNTIME_SYSTEM_EVENT_EVENTFD_EVENT_HPP_

