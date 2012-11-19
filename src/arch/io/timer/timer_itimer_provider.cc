#include "arch/io/timer/timer_itimer_provider.hpp"
#include "arch/io/timer_provider.hpp"

#include <sys/time.h>
#include <string.h>

timer_itimer_provider_t::timer_itimer_provider_t(linux_event_queue_t *queue,
                                                 timer_provider_callback_t *callback,
                                                 UNUSED time_t secs, UNUSED int32_t nsecs)
    : queue_(queue), callback_(callback) {
    // Tell the event queue to start watching the signal.
    sigevent evp;
    evp.sigev_signo = SIGALRM;
    // TODO(OSX): How does the timer signal ever get back to this?  watch_signal ignores the "this" parameter.
    queue_->watch_signal(&evp, this);

    // Create the timer.
    // TODO(OSX) Actually support timing things.
    // struct itimerval value;
    // value.it_interval.tv_sec = secs;
    // value.it_interval.tv_usec = nsecs;
    // value.it_value = value.it_interval;

    // struct itimerval old_value;

    // int res = setitimer(ITIMER_REAL, &value, &old_value);
    // guarantee_err(res == 0, "setitimer failed");
    // guarantee(old_value.it_interval.tv_sec == 0 && old_value.it_interval.tv_usec == 0
    //           && old_value.it_value.tv_sec == 0 && old_value.it_value.tv_usec == 0,
    //           "setitimer reported that the old timer was present");
}

timer_itimer_provider_t::~timer_itimer_provider_t() {
    // TODO(OSX) Actually support timing things.
    // struct itimerval value;
    // value.it_interval.tv_sec = 0;
    // value.it_interval.tv_usec = 0;
    // value.it_value = value.it_interval;

    // struct itimerval old_value;
    // int res = setitimer(ITIMER_REAL, &value, &old_value);
    // guarantee_err(res == 0, "setitimer failed");
    // guarantee(old_value.it_interval.tv_sec != 0 || old_value.it_interval.tv_usec != 0,
    //           "setitimer reported that the old timer was present");
}

void timer_itimer_provider_t::on_event(int events) {
    // TODO(OSX): This change is bullshit so far, in order to trick ourselves into compiling at all.
    // This callback never actually gets called.

    // TODO(OSX): See the comment in timer_signal_provider_t, we just made the same assumption here
    // and passed events + 1.
    callback_->on_timer(events + 1);
}
