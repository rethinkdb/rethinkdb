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
#include "utils.hpp"

// This *should* be a member of sigevent exposed by glibc, who the
// heck knows why it isn't...
#define sigev_notify_thread_id _sigev_un._tid

void timer_signal_provider_signal_handler(UNUSED int signum, siginfo_t *siginfo, UNUSED void *uctx) {
    timer_signal_provider_t *provider = static_cast<timer_signal_provider_t *>(siginfo->si_value.sival_ptr);

    timer_provider_callback_t *local_cb = provider->callback;
    if (local_cb != nullptr) {
        provider->callback = nullptr;
        local_cb->on_oneshot();
    }
}

/* Kernel timer provider based on signals */
timer_signal_provider_t::timer_signal_provider_t(UNUSED linux_event_queue_t *queue)
    : callback(NULL) {
    struct sigaction sa = make_sa_sigaction(SA_SIGINFO, &timer_signal_provider_signal_handler);

    // Register the signal.
    int res = sigaction(TIMER_NOTIFY_SIGNAL, &sa, nullptr);
    guarantee_err(res == 0, "timer signal provider could not register the signal handler");

    // Initialize the event structure
    struct sigevent evp;
    bzero(&evp, sizeof(sigevent));
    evp.sigev_signo = TIMER_NOTIFY_SIGNAL;
    evp.sigev_notify = SIGEV_THREAD_ID;
    evp.sigev_notify_thread_id = _gettid();
    evp.sigev_value.sival_ptr = this;

    // Create the timer
    res = timer_create(CLOCK_MONOTONIC, &evp, &timerid);
    guarantee_err(res == 0, "Could not create timer");
}

timer_signal_provider_t::~timer_signal_provider_t() {
    guarantee(callback == nullptr);

    int res = timer_delete(timerid);
    guarantee_err(res == 0, "timer_delete failed");

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;

    res = sigaction(TIMER_NOTIFY_SIGNAL, &sa, nullptr);
    guarantee_err(res == 0, "timer signal provider could not unregister the signal handler");
}

void timer_signal_provider_t::schedule_oneshot(int64_t next_time_in_nanos, timer_provider_callback_t *cb) {
    // We could ostensibly use TIMER_ABSTIME in the timer_settime call instead of specifying a
    // relative timer, but that would make our code fragilely depend on get_ticks() using
    // CLOCK_MONOTONIC.

    const int64_t ticks = get_ticks();
    const int64_t time_diff = next_time_in_nanos - ticks;
    const int64_t wait_time = std::max<int64_t>(1, time_diff);

    itimerspec spec;
    spec.it_value.tv_sec = wait_time / BILLION;
    spec.it_value.tv_nsec = wait_time % BILLION;
    spec.it_interval.tv_sec = 0;
    spec.it_interval.tv_nsec = 0;

    const int res = timer_settime(timerid, 0, &spec, nullptr);
    guarantee_err(res == 0, "Could not arm the timer");

    callback = cb;
}

void timer_signal_provider_t::unschedule_oneshot() {
    callback = nullptr;

    struct itimerspec spec;
    spec.it_interval.tv_sec = 0;
    spec.it_interval.tv_nsec = 0;
    spec.it_value.tv_sec = 0;
    spec.it_value.tv_nsec = 0;

    const int res = timer_settime(timerid, 0, &spec, nullptr);
    guarantee_err(res == 0, "Could not disarm the timer.");
}

#endif  // RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_SIGNAL
