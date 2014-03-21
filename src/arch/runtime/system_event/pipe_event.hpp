// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_SYSTEM_EVENT_PIPE_EVENT_HPP_
#define ARCH_RUNTIME_SYSTEM_EVENT_PIPE_EVENT_HPP_

#include <stdint.h>

#include "errors.hpp"

struct pipe_event_t {
public:
    pipe_event_t();
    ~pipe_event_t();

    void consume_wakey_wakeys();

    // You may call this from any thread (as long as you know nobody will destroy the
    // eventfd_event_t object simultaneously).
    void wakey_wakey();

    int get_notify_fd() {
        return read_fd_;
    }

private:
    int read_fd_;
    int write_fd_;
    DISABLE_COPYING(pipe_event_t);
};


#endif // ARCH_RUNTIME_SYSTEM_EVENT_PIPE_EVENT_HPP_

