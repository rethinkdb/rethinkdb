// Copyright 2015 RethinkDB, all rights reserved.
#include "arch/io/timer_provider.hpp"

#if RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_EVPORT

#include "arch/io/timer/timer_evport_provider.hpp"

#include <strings.h>

#include "arch/runtime/event_queue.hpp"
#include "arch/io/io_utils.hpp"
#include "utils.hpp"
#include "logger.hpp"

/* Kernel timer provider based on event ports */
timer_evport_provider_t::timer_evport_provider_t(linux_event_queue_t *_queue)
    : queue(_queue), callback(NULL) {

    port_notify_t pn;
    struct sigevent evp;
    int res;

    bzero(&pn, sizeof(port_notify_t));
    pn.portnfy_port = queue->get_evport_fd();
    pn.portnfy_user = this;

    bzero(&evp, sizeof(sigevent));
    evp.sigev_notify = SIGEV_PORT;
    evp.sigev_value.sival_ptr = &pn;

    /*
     * Create the timer.  Ideally we'd use CLOCK_HIGHRES here but its use
     * is restricted, notably in zones.
     */
    do {
        res = timer_create(CLOCK_REALTIME, &evp, &timerid);
    } while (res == -1 && get_errno() == EAGAIN);
    guarantee_err(res == 0, "Could not create timer");
}

timer_evport_provider_t::~timer_evport_provider_t() {
    guarantee(callback == NULL);

    int res = timer_delete(timerid);
    guarantee_err(res == 0, "timer_delete failed");
}

void timer_evport_provider_t::schedule_oneshot(int64_t next_time_in_nanos, timer_provider_callback_t *cb) {
    const int64_t ticks = get_ticks();
    const int64_t time_diff = next_time_in_nanos - ticks;
    const int64_t wait_time = std::max<int64_t>(1, time_diff);

    itimerspec spec;
    spec.it_value.tv_sec = wait_time / BILLION;
    spec.it_value.tv_nsec = wait_time % BILLION;
    spec.it_interval.tv_sec = 0;
    spec.it_interval.tv_nsec = 0;

    const int res = timer_settime(timerid, 0, &spec, NULL);
    guarantee_err(res == 0, "Could not arm the timer");

    callback = cb;
}

void timer_evport_provider_t::unschedule_oneshot() {
    struct itimerspec spec;
    spec.it_interval.tv_sec = 0;
    spec.it_interval.tv_nsec = 0;
    spec.it_value.tv_sec = 0;
    spec.it_value.tv_nsec = 0;

    const int res = timer_settime(timerid, 0, &spec, NULL);
    guarantee_err(res == 0, "Could not disarm the timer.");

    callback = NULL;
}

void timer_evport_provider_t::on_event(int eventmask) {
    if (eventmask != poll_event_in) {
        logERR("Unexpected event mask: %d", eventmask);
    }
    if (callback != NULL) {
        timer_provider_callback_t *local_cb = callback;
        callback = NULL;
        local_cb->on_oneshot();
    }
}

#endif  // RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_EVPORT
