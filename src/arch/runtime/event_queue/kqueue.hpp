// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_EVENT_QUEUE_KQUEUE_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_KQUEUE_HPP_

#include <sys/event.h>

#include <map>
#include <set>

#include "arch/runtime/event_queue_types.hpp"
#include "arch/runtime/runtime_utils.hpp"
#include "config/args.hpp"

// Event queue using `kqueue`/`kevent`
class kqueue_event_queue_t {
public:
    explicit kqueue_event_queue_t(linux_queue_parent_t *parent);
    void run();
    ~kqueue_event_queue_t();

    // These should only be called by the event queue itself or by the linux_* classes
    void watch_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void adjust_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void forget_resource(fd_t resource, linux_event_callback_t *cb);

private:
    // Helper functions
    void add_filters(fd_t resource, const std::set<int16_t> &filters,
                     linux_event_callback_t *cb);
    void del_filters(fd_t resource, const std::set<int16_t> &filters,
                     linux_event_callback_t *cb);

    linux_queue_parent_t *parent;

    fd_t kqueue_fd;

    // Keep track of which event types we are currently watching for a resource,
    // so that `adjust_resource` can generate an appropriate diff and
    // `forget_resource` knows which events to unwatch.
    std::map<fd_t, int> watched_events;

    // We store this as a class member because forget_resource needs
    // to go through the events and remove queued messages for
    // resources that are being destroyed.
    struct kevent events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];
    int nevents;

    DISABLE_COPYING(kqueue_event_queue_t);
};


#endif // ARCH_RUNTIME_EVENT_QUEUE_KQUEUE_HPP_
