// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/io/timer_provider.hpp"

#if RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_SIGNAL

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

void timer_signal_provider_signal_handler(UNUSED int signum, siginfo_t *siginfo, UNUSED void *uctx) {
    timer_provider_callback_t *callback = static_cast<timer_provider_callback_t *>(siginfo->si_value.sival_ptr);

    // The number of timer ticks that have happened is 1 plus the timer signal's overrun count.
    callback->on_timer(1 + siginfo->si_overrun);
}

/* Kernel timer provider based on signals */
timer_signal_provider_t::timer_signal_provider_t(UNUSED linux_event_queue_t *queue,
                                                 timer_provider_callback_t *callback,
                                                 time_t secs, int32_t nsecs) {
    struct sigaction sa = make_sa_sigaction(SA_SIGINFO, &timer_signal_provider_signal_handler);

    // Register the signal.
    int res = sigaction(TIMER_NOTIFY_SIGNAL, &sa, NULL);
    guarantee_err(res == 0, "timer signal provider could not register the signal handler");

    // Initialize the event structure
    struct sigevent evp;
    bzero(&evp, sizeof(sigevent));
    evp.sigev_signo = TIMER_NOTIFY_SIGNAL;
    evp.sigev_notify = SIGEV_THREAD_ID;
    evp.sigev_notify_thread_id = _gettid();
    evp.sigev_value.sival_ptr = callback;

    // Create the timer
    res = timer_create(CLOCK_MONOTONIC, &evp, &timerid);
    guarantee_err(res == 0, "Could not create timer");

    // Start the timer
    itimerspec timer_spec;
    bzero(&timer_spec, sizeof(timer_spec));
    timer_spec.it_value.tv_sec = timer_spec.it_interval.tv_sec = secs;
    timer_spec.it_value.tv_nsec = timer_spec.it_interval.tv_nsec = nsecs;

    res = timer_settime(timerid, 0, &timer_spec, NULL);
    guarantee_err(res == 0, "Could not arm the timer");
}

timer_signal_provider_t::~timer_signal_provider_t() {
    int res = timer_delete(timerid);
    guarantee_err(res == 0, "timer_delete failed");

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;

    res = sigaction(TIMER_NOTIFY_SIGNAL, &sa, NULL);
    guarantee_err(res == 0, "timer signal provider could not unregister the signal handler");
}

#endif  // RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_SIGNAL
