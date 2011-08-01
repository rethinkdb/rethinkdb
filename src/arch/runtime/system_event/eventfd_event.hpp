#ifndef __ARCH_RUNTIME_EVENTFD_EVENT_HPP__
#define __ARCH_RUNTIME_EVENTFD_EVENT_HPP__

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

#endif // __ARCH_RUNTIME_EVENTFD_EVENT_HPP__

