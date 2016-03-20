// Copyright 2010-2014 RethinkDB, all rights reserved.
#if defined(__MACH__)

#include "arch/runtime/event_queue/kqueue.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include <set>

#include "arch/runtime/event_queue.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "config/args.hpp"
#include "errors.hpp"
#include "perfmon/perfmon.hpp"
#include "utils.hpp"

std::set<int16_t> user_to_kevent_filters(int mode) {
    rassert((mode & (poll_event_in | poll_event_out)) == mode);

    std::set<int16_t> filters;
    if (mode & poll_event_in) filters.insert(EVFILT_READ);
    if (mode & poll_event_out) filters.insert(EVFILT_WRITE);

    return filters;
}

int kevent_filter_to_user(int16_t filter) {
    rassert(filter == EVFILT_READ || filter == EVFILT_WRITE);

    if (filter == EVFILT_READ) return poll_event_in;
    if (filter == EVFILT_WRITE) return poll_event_out;
    unreachable();
}

kqueue_event_queue_t::kqueue_event_queue_t(linux_queue_parent_t *_parent)
    : parent(_parent) {

    // Create the kqueue
    kqueue_fd = kqueue();
    guarantee_err(kqueue_fd >= 0, "Could not create kqueue");
}

// Small helper function to call `kevent` with error checking
int call_kevent(int kq, const struct kevent *changelist, int nchanges,
                struct kevent *eventlist, int nevents,
                const struct timespec *timeout) {
    int res;
    do {
        res = kevent(kq, changelist, nchanges, eventlist, nevents, timeout);
    } while (res == -1 && get_errno() == EINTR);

    // Apart from EINTR, it doesn't seem like there's any error code that we can
    // really handle here. Terminate instead if the call failed.
    guarantee_err(res != -1, "Call to kevent() failed");

    return res;
}

void kqueue_event_queue_t::run() {
    // Now, start the loop
    while (!parent->should_shut_down()) {
        // Grab the events from the kqueue!
        nevents = call_kevent(kqueue_fd, NULL, 0,
                              events, MAX_IO_EVENT_PROCESSING_BATCH_SIZE, NULL);

        block_pm_duration event_loop_timer(pm_eventloop_singleton_t::get());

        for (int i = 0; i < nevents; i++) {
            if (events[i].udata == NULL) {
                // The event was queued for a resource that's
                // been destroyed, so forget_resource is kindly
                // notifying us to skip it by setting `udata` to NULL.
                continue;
            } else {
                linux_event_callback_t *cb =
                    reinterpret_cast<linux_event_callback_t *>(events[i].udata);
                cb->on_event(kevent_filter_to_user(events[i].filter));
            }
        }

        parent->pump();
    }
}

kqueue_event_queue_t::~kqueue_event_queue_t() {
    int res = close(kqueue_fd);
    guarantee_err(res == 0, "Could not close kqueue");
}

void kqueue_event_queue_t::add_filters(fd_t resource, const std::set<int16_t> &filters,
                                       linux_event_callback_t *cb) {
    rassert(cb != NULL);

    for (auto filter : filters) {
        struct kevent ev;
        EV_SET(&ev, static_cast<uintptr_t>(resource), filter, EV_ADD, 0, 0, cb);
        call_kevent(kqueue_fd, &ev, 1, NULL, 0, NULL);
    }
}

void kqueue_event_queue_t::del_filters(fd_t resource, const std::set<int16_t> &filters,
                                       linux_event_callback_t *cb) {
    rassert(cb != NULL);

    for (auto filter : filters) {
        struct kevent ev;
        EV_SET(&ev, static_cast<uintptr_t>(resource), filter, EV_DELETE, 0, 0, cb);
        call_kevent(kqueue_fd, &ev, 1, NULL, 0, NULL);
    }
}

void kqueue_event_queue_t::watch_resource(fd_t resource, int event_mask,
                                          linux_event_callback_t *cb) {
    rassert(cb != NULL);

    // Start watching the events on `resource`
    std::set<int16_t> filters = user_to_kevent_filters(event_mask);
    add_filters(resource, filters, cb);

    guarantee(watched_events.count(resource) == 0);
    watched_events[resource] = event_mask;
}

void kqueue_event_queue_t::adjust_resource(fd_t resource, int event_mask,
                                           linux_event_callback_t *cb) {
    rassert(cb);

    auto ev = watched_events.find(resource);
    guarantee(ev != watched_events.end());

    const std::set<int16_t> current_filters = user_to_kevent_filters(ev->second);
    const std::set<int16_t> new_filters = user_to_kevent_filters(event_mask);

    // We generate a diff to find out which filters we have to delete from the
    // kqueue.
    // On the other hand we re-add all events, which will cause kqueue to update the
    // `udata` field (the callback) of the existing events in case it has changed.
    std::set<int16_t> filters_to_del = current_filters;
    for (auto f : new_filters) {
        filters_to_del.erase(f);
    }

    // Apply the diff
    del_filters(resource, filters_to_del, cb);
    add_filters(resource, new_filters, cb);
    ev->second = event_mask;

    // Go through the queue of messages in the current poll cycle and
    // clean out the ones that are referencing the filters that are being
    // deleted.
    //
    // We also update the cb, in case we have changed it
    // Note: we don't currently support changing the cb through `adjust_resource`
    //   in the epoll queue implementation. It is supported in the poll one though,
    //   and there's no reason for why we should not support it here.
    for (int i = 0; i < nevents; i++) {
        if (events[i].ident == static_cast<uintptr_t>(resource)) {
            if (filters_to_del.count(events[i].filter) > 0) {
                // No longer watching this event
                events[i].udata = NULL;
            } else if (events[i].udata != NULL) {
                // Just update the cb, unless we had previously deleted this one.
                events[i].udata = cb;
            }
        }
    }
}

void kqueue_event_queue_t::forget_resource(fd_t resource, linux_event_callback_t *cb) {
    rassert(cb);

    // To stop watching all associated events, we have to re-generate the
    // corresponding filters. `kqueue` identified an event by its `ident`, `filter`,
    // pair so we generate an EV_DELETE kevent for each one.
    auto ev = watched_events.find(resource);
    guarantee(ev != watched_events.end());
    std::set<int16_t> filters = user_to_kevent_filters(ev->second);
    del_filters(resource, filters, cb);
    watched_events.erase(ev);

    // Go through the queue of messages in the current poll cycle and
    // clean out the ones that are referencing the resource we're
    // being asked to forget.
    for (int i = 0; i < nevents; i++) {
        if (events[i].ident == static_cast<uintptr_t>(resource)) {
            rassert(events[i].udata == cb || events[i].udata == NULL);
            events[i].udata = NULL;
        }
    }
}

void kqueue_event_queue_t::watch_event(system_event_t *ev, linux_event_callback_t *cb) {
    watch_resource(ev->get_notify_fd(), poll_event_in, cb);
}

void kqueue_event_queue_t::forget_event(system_event_t *ev, linux_event_callback_t *cb) {
    forget_resource(ev->get_notify_fd(), cb);
}

#endif // defined(__MACH__)
