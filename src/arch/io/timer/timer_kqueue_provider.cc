// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/io/timer_provider.hpp"

#if RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_KQUEUE

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <inttypes.h>

#include "config/args.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include "time.hpp"
#include "containers/printf_buffer.hpp"

timer_kqueue_provider_t::timer_kqueue_provider_t(linux_event_queue_t *queue)
    : queue_(queue), kq_fd_(-1), callback_(NULL) {
    const int fd = kqueue();
    guarantee_err(fd != -1, "kqueue() call falied");
    kq_fd_ = fd;
    queue_->watch_resource(kq_fd_, poll_event_in, this);
}

timer_kqueue_provider_t::~timer_kqueue_provider_t() {
    queue_->forget_resource(kq_fd_, this);

    guarantee(callback_ == nullptr);
    const int res = close(kq_fd_);
    guarantee_err(res == 0, "Could not close the kqueue timer.");
}

void timer_kqueue_provider_t::schedule_oneshot(const int64_t next_time_in_nanos, timer_provider_callback_t *const cb) {
    const int64_t time_difference = next_time_in_nanos - static_cast<int64_t>(get_ticks());
    const int64_t wait_nanos = std::max<int64_t>(1, time_difference);

    struct kevent64_s event;
    EV_SET64(&event, 99, EVFILT_TIMER, EV_ADD | EV_ONESHOT, NOTE_NSECONDS, wait_nanos, 0, 0, 0);

    // Setting the timeout to zero makes this a non-blocking call.
    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;
    const int res = kevent64(kq_fd_, &event, 1, nullptr, 0, 0, &timeout);
    guarantee_err(res != -1, "kevent64 failed when making timer");

    callback_ = cb;
}

void timer_kqueue_provider_t::unschedule_oneshot() {
    struct kevent64_s event;
    EV_SET64(&event, 99, EVFILT_TIMER, EV_DELETE, 0, 0, 0, 0, 0);

    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;
    const int res = kevent64(kq_fd_, &event, 1, nullptr, 0, 0, &timeout);
    guarantee_err(res != -1, "kevent64 failed when deleting timer");

    callback_ = nullptr;
}

void debug_print(printf_buffer_t *buf, const struct kevent64_s& event) {
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
            res = kevent64(kq_fd_, nullptr, 0, events, num_events, 0, &timeout);
        } while (res == -1 && get_errno() == EINTR);

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

    if (expiration_count > 0 && callback_ != nullptr) {
        // Make the callback be NULL before we call it, so that a new callback can be set.
        timer_provider_callback_t *local_cb = callback_;
        callback_ = nullptr;
        local_cb->on_oneshot();
    }
}

#endif  // RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_KQUEUE
