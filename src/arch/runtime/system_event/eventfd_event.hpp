// Copyright 2010-2012 RethinkDB, all rights reserved.
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

    uint64_t read();
    void write(uint64_t value);

    int get_notify_fd() {
        return eventfd_;
    }

private:
    int eventfd_;
    DISABLE_COPYING(eventfd_event_t);
};

#endif // ARCH_RUNTIME_SYSTEM_EVENT_EVENTFD_EVENT_HPP_

