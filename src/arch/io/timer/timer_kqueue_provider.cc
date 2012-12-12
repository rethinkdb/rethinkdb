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

void debug_print(append_only_printf_buffer_t *buf, const struct kevent64_s& event) {
    buf->appendf("kevent64_s{ident=%" PRIu64 ", filter=%" PRIi16 ", flags=%" PRIu16
                 ", fflags=%" PRIu32 ", data=%" PRIi64 ", udata=%" PRIu64
                 ", ext[0]=%" PRIu64 ", ext[1]=%" PRIu64,
                 event.ident, event.filter, event.flags,
                 event.fflags, event.data, event.udata,
                 event.ext[0], event.ext[1]);
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

    // kevent64 can return more than one EVFILT_TIMER event at a time, even though we only
    // registered for it once and we can get an overrun count.  We get multiple results for the same
    // event, separately.  So we have this loop that makes sure we siphon all the possible events
    // off the kqueue file descriptor.  The only time I've seen it return more than one event was
    // when sleeping the computer, and another time when leaving the computer idle for a while.
    int64_t expiration_count = 0;

    bool got_short_res = false;
    while (!got_short_res) {
        int res;
        do {
            res = kevent64(kq_fd_, NULL, 0, events, num_events, 0, &timeout);
        } while (res == -1 && errno == EINTR);

        guarantee_err(res != -1, "kevent64 failed when reading timer");

        got_short_res = res < num_events;

        for (int i = 0; i < res; ++i) {
            const struct kevent64_s event = events[i];
            guarantee(event.filter == EVFILT_TIMER);
            guarantee_xerr(!(event.flags & EV_ERROR), event.data, "The kqueue-based timer returned an error.\n");

            guarantee(event.ident == 99);
            int64_t this_result_expiration_count = events[i].data;
            expiration_count += this_result_expiration_count;
        }
    }

    callback_->on_timer(expiration_count);
}

#endif  // RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_KQUEUE
