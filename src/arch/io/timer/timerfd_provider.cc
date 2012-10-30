// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef LEGACY_LINUX // So the build system doesn't try to compile this file otherwise.

#include "arch/io/timer/timerfd_provider.hpp"

#include <sys/timerfd.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/eventfd.h>

#include "arch/runtime/event_queue.hpp"
#include "arch/io/timer_provider.hpp"
#include "logger.hpp"

/* Kernel timer provider based on timerfd  */
timerfd_provider_t::timerfd_provider_t(linux_event_queue_t *_queue,
                                       timer_provider_callback_t *_callback,
                                       time_t secs, int32_t nsecs)
    : queue(_queue), callback(_callback)
{
    int res;

    timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    guarantee_err(timer_fd != -1, "Could not create timer");

    res = fcntl(timer_fd, F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make timer non-blocking");

    itimerspec timer_spec;
    bzero(&timer_spec, sizeof(timer_spec));
    timer_spec.it_value.tv_sec = timer_spec.it_interval.tv_sec = secs;
    timer_spec.it_value.tv_nsec = timer_spec.it_interval.tv_nsec = nsecs;

    res = timerfd_settime(timer_fd, 0, &timer_spec, NULL);
    guarantee_err(res == 0, "Could not arm the timer");

    queue->watch_resource(timer_fd, poll_event_in, this);
}

timerfd_provider_t::~timerfd_provider_t() {
    queue->forget_resource(timer_fd, this);
    int res = close(timer_fd);
    guarantee_err(res == 0, "Could not close the timer.");
}

void timerfd_provider_t::on_event(int events) {
    if (events != poll_event_in) {
        logERR("Unexpected event mask: %d", events);
    }

    int res;
    eventfd_t nexpirations;

    res = eventfd_read(timer_fd, &nexpirations);
    guarantee_err(res == 0 || errno == EAGAIN, "Could not read timer_fd value");
    if (res == 0) {
        callback->on_timer(nexpirations);
    }
}

#endif
