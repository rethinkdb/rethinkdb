// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_EVENT_QUEUE_EPOLL_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_EPOLL_HPP_

#include <sys/epoll.h>

#ifndef NDEBUG
#include <map>
#endif

#include "arch/runtime/event_queue_types.hpp"
#include "arch/runtime/runtime_utils.hpp"
#include "arch/runtime/system_event.hpp"
#include "config/args.hpp"

// Event queue structure
struct epoll_event_queue_t {
public:
    explicit epoll_event_queue_t(linux_queue_parent_t *parent);
    void run();
    ~epoll_event_queue_t();

    // These should only be called by the event queue itself or by the
    // linux_* classes
    void watch_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void adjust_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void forget_resource(fd_t resource, linux_event_callback_t *cb);

    void watch_event(system_event_t *, linux_event_callback_t *cb);
    void forget_event(system_event_t *, linux_event_callback_t *cb);

private:
    linux_queue_parent_t *parent;

    fd_t epoll_fd;

    // We store this as a class member because forget_resource needs
    // to go through the events and remove queued messages for
    // resources that are being destroyed.
    epoll_event events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];
    int nevents;

#ifndef NDEBUG
    /* In debug mode, check to make sure epoll() doesn't give us events that
    we didn't ask for. The ints stored here are combinations of poll_event_in
    and poll_event_out. */
    std::map<linux_event_callback_t*, int> events_requested;
#endif

    DISABLE_COPYING(epoll_event_queue_t);
};


#endif // ARCH_RUNTIME_EVENT_QUEUE_EPOLL_HPP_
