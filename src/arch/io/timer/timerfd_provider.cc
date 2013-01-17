// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/io/timer_provider.hpp"

#if RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_TIMERFD

#include <fcntl.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

#include "arch/runtime/event_queue.hpp"
#include "arch/io/timer_provider.hpp"
#include "logger.hpp"
#include "utils.hpp"

timerfd_provider_t::timerfd_provider_t(linux_event_queue_t *_queue)
    : queue(_queue), timer_fd(-1), callback(NULL) {
    const int fd = timerfd_create(CLOCK_MONOTONIC, 0);
    guarantee_err(fd != -1, "Could not create timer");

    const int res = fcntl(fd, F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make timer non-blocking");

    timer_fd = fd;

    // We watch the resource, but so far we haven't set any timings.
    queue->watch_resource(timer_fd, poll_event_in, this);
}

timerfd_provider_t::~timerfd_provider_t() {
    queue->forget_resource(timer_fd, this);

    guarantee(callback == NULL);
    const int res = close(timer_fd);
    guarantee_err(res == 0 || errno == EINTR, "Could not close the timer.");
}

void timerfd_provider_t::schedule_oneshot(const uint64_t next_time_in_nanos, timer_provider_interactor_t *const cb) {
    // We could pass TFD_TIMER_ABSTIME to timerfd_settime (thus avoiding the std::max logic below),
    // but that would mean this code depends on the fact that get_ticks() is implemented in terms of
    // CLOCK_MONOTONIC.

    const int64_t time_difference = static_cast<int64_t>(next_time_in_nanos) - static_cast<int64_t>(get_ticks());
    const int64_t wait_nanos = std::max<int64_t>(1, time_difference);

    struct itimerspec spec;
    spec.it_interval.tv_sec = 0;
    spec.it_interval.tv_nsec = 0;
    spec.it_value.tv_sec = wait_nanos / BILLION;
    spec.it_value.tv_nsec = wait_nanos % BILLION;


    int res = timerfd_settime(timer_fd, 0, &spec, NULL);
    guarantee_err(res == 0, "Could not arm the timer.");

    callback = cb;
}

void timerfd_provider_t::unschedule_oneshot() {
    struct itimerspec spec;
    spec.it_interval.tv_sec = 0;
    spec.it_interval.tv_nsec = 0;
    spec.it_value.tv_sec = 0;
    spec.it_value.tv_nsec = 0;

    int res = timerfd_settime(timer_fd, 0, &spec, NULL);
    guarantee_err(res == 0, "Could not disarm the timer.");

    callback = NULL;
}

void timerfd_provider_t::on_event(int events) {
    if (events != poll_event_in) {
        logERR("Unexpected event mask: %d", events);
    }

    eventfd_t nexpirations;
    const int res = eventfd_read(timer_fd, &nexpirations);
    guarantee_err(res == 0 || errno == EAGAIN, "Could not read timer_fd value");
    if (res == 0 && nexpirations > 0) {
        timer_provider_interactor_t *local_cb = callback;
        callback = NULL;
        local_cb->on_oneshot();
    }
}

#endif  // RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_TIMERFD
