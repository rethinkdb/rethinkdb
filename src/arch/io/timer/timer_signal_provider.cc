#include "arch/io/timer/timer_signal_provider.hpp"

#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "arch/runtime/event_queue.hpp"
#include "arch/io/io_utils.hpp"
#include "arch/io/timer_provider.hpp"

// This *should* be a member of sigevent exposed by glibc, who the
// heck knows why it isn't...
#define sigev_notify_thread_id _sigev_un._tid

/* Kernel timer provider based on signals */
timer_signal_provider_t::timer_signal_provider_t(linux_event_queue_t *_queue,
                                                 timer_provider_callback_t *_callback,
                                                 time_t secs, int32_t nsecs)
    : queue(_queue), callback(_callback)
{
    // Initialize the event structure
    bzero(&evp, sizeof(sigevent));
    evp.sigev_signo = TIMER_NOTIFY_SIGNAL;
    evp.sigev_notify = SIGEV_THREAD_ID;
    evp.sigev_notify_thread_id = _gettid();
    evp.sigev_value.sival_ptr = this;

    // Tell the event queue to start watching the signal. It's
    // important to call this before creating the timer, because the
    // event queue automatically blocks the signal (which is necessary
    // to implement poll/epoll properly).
    queue->watch_signal(&evp, this);

    // Create the timer
    int res = timer_create(CLOCK_MONOTONIC, &evp, &timerid);
    guaranteef_err(res == 0, "Could not create timer");

    // Arm the timer
    itimerspec timer_spec;
    bzero(&timer_spec, sizeof(timer_spec));
    timer_spec.it_value.tv_sec = timer_spec.it_interval.tv_sec = secs;
    timer_spec.it_value.tv_nsec = timer_spec.it_interval.tv_nsec = nsecs;

    res = timer_settime(timerid, 0, &timer_spec, NULL);
    guaranteef_err(res == 0, "Could not arm the timer");
}

timer_signal_provider_t::~timer_signal_provider_t() {
    int res = timer_delete(timerid);
    guaranteef_err(res == 0, "Could not close the timer.");
    queue->forget_signal(&evp, this);
}

void timer_signal_provider_t::on_event(int events) {
    // In case of signals, events argument accepts the number of times
    // the event was overrun. For signal-based timers this is zero in
    // most cases, so we should add one when we call the timer
    // callback api.
    callback->on_timer(events + 1);
}

