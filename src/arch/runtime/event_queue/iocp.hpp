// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_EVENT_QUEUE_EPOLL_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_EPOLL_HPP_

#ifndef NDEBUG
#include <map>
#endif

#include "arch/runtime/event_queue_types.hpp"
// ATN TODO

#include "arch/runtime/runtime_utils.hpp"
#include "config/args.hpp"

// Event queue structure
struct iocp_event_queue_t {
public:
    explicit iocp_event_queue_t(linux_queue_parent_t *parent);
    void run();
    ~iocp_event_queue_t();

    // These should only be called by the event queue itself or by the
    // linux_* classes
    void watch_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void adjust_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void forget_resource(fd_t resource, linux_event_callback_t *cb);

private:
	// ATN

    DISABLE_COPYING(iocp_event_queue_t);
};


#endif // ARCH_RUNTIME_EVENT_QUEUE_EPOLL_HPP_
