// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/io/timer_provider.hpp"

#if RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_KQUEUE

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include "config/args.hpp"
#include "logger.hpp"
#include "utils.hpp"

timer_kqueue_provider_t::timer_kqueue_provider_t(linux_event_queue_t *queue,
                                                 timer_provider_callback_t *callback,
                                                 time_t secs, int32_t nsecs)
    : queue_(queue), callback_(callback), kq_fd_(-1) {

    // We can't make the kqueue a non-blocking file descriptor -- we do non-blocking kevent64 calls.
    int fd = kqueue();
    guarantee_err(fd != -1, "kqueue() call failed");

    uint64_t nsecs64 = static_cast<uint64_t>(secs) * BILLION + nsecs;

    struct kevent64_s event;
    EV_SET64(&event, 99, EVFILT_TIMER, EV_ADD, NOTE_NSECONDS, nsecs64, 0, 0, 0);

    // Setting the timeout to zero makes this a non-blocking call.
    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;
    int res = kevent64(fd, &event, 1, NULL, 0, 0, &timeout);
    guarantee_err(res != -1, "kevent64 failed when making timer");

    kq_fd_ = fd;

    queue_->watch_resource(kq_fd_, poll_event_in, this);
}

timer_kqueue_provider_t::~timer_kqueue_provider_t() {
    queue_->forget_resource(kq_fd_, this);

    int res = close(kq_fd_);
    guarantee_err(res == 0, "Could not close the kqueue timer.");
}

void timer_kqueue_provider_t::on_event(int eventmask) {
    if (eventmask != poll_event_in) {
        logERR("Unexpected event mask: %d", eventmask);
    }

    const int num_events = 4;
    struct kevent64_s events[num_events];

    // Setting the timeout to 0 makes this a non-blocking call.
    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;
    int res;
    do {
        res = kevent64(kq_fd_, NULL, 0, events, num_events, 0, &timeout);
    } while (res == -1 && errno == EINTR);

    guarantee_err(res != -1, "kevent64 failed when reading timer");

    // There _should_ be one result (because we got notified by the thread pool's event queue) and
    // there definitely should not be more than one result (because we only registered for one
    // event), but there _could_ be zero results if we got an unexpected event mask, which we only
    // handle by logging at hte top of this function.
    guarantee(res <= 1);

    for (int i = 0; i < res; ++i) {
        const struct kevent64_s event = events[i];
        guarantee(event.filter == EVFILT_TIMER);
        guarantee_xerr(!(event.flags & EV_ERROR), event.data, "The kqueue-based timer returned an error.\n");

        guarantee(event.ident == 99);
        int64_t expiration_count = events[i].data;
        callback_->on_timer(expiration_count);
    }
}

#endif  // RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_KQUEUE
