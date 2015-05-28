// Copyright 2015 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_EVENT_QUEUE_EVPORT_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_EVPORT_HPP_

#include <poll.h>
#include <port.h>

#include "arch/runtime/event_queue_types.hpp"
#include "arch/runtime/runtime_utils.hpp"
#include "config/args.hpp"

// Event queue structure
struct evport_event_queue_t {
public:
    explicit evport_event_queue_t(linux_queue_parent_t *parent);
    void run();
    ~evport_event_queue_t();

    // These should only be called by the event queue itself or by the
    // linux_* classes
    void watch_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void adjust_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void forget_resource(fd_t resource, linux_event_callback_t *cb);

    fd_t get_evport_fd();

private:
    linux_queue_parent_t *parent;

    fd_t evport_fd;
    port_event_t events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];
    uint_t nevents;

    DISABLE_COPYING(evport_event_queue_t);
};


#endif // ARCH_RUNTIME_EVENT_QUEUE_EVPORT_HPP_
