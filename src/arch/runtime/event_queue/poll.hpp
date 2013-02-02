// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_EVENT_QUEUE_POLL_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_POLL_HPP_

#include <poll.h>

#include <map>
#include <vector>

#include "arch/runtime/event_queue_types.hpp"
#include "arch/runtime/runtime_utils.hpp"

// Event queue structure
class poll_event_queue_t {
public:
    explicit poll_event_queue_t(linux_queue_parent_t *parent);
    void run();
    ~poll_event_queue_t();

    // These should only be called by the event queue itself or by the linux_* classes
    void watch_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void adjust_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void forget_resource(fd_t resource, linux_event_callback_t *cb);

private:
    linux_queue_parent_t *parent;

    std::vector<pollfd> watched_fds;
    std::map<fd_t, linux_event_callback_t *> callbacks;
};


#endif // ARCH_RUNTIME_EVENT_QUEUE_POLL_HPP_
