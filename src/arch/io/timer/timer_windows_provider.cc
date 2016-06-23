// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/io/timer_provider.hpp"

#if RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_WINDOWS

#include <inttypes.h>

#include "logger.hpp"
#include "time.hpp"

#ifdef TRACE_TIMER
#define debugf_timer(...) debugf("timer: " __VA_ARGS__)
#else
#define debugf_timer(...) ((void)0)
#endif

timer_windows_provider_t::timer_windows_provider_t(iocp_event_queue_t *event_queue_) :
    callback(nullptr), event_queue(event_queue_) {
    debugf_timer("[%p] create\n", this);
    // TODO WINDOWS: assert that there is no other timer provider for this thread
}

timer_windows_provider_t::~timer_windows_provider_t() {
    debugf_timer("[%p] destroy\n", this);
    unschedule_oneshot();
}

void timer_windows_provider_t::schedule_oneshot(const int64_t next_time_in_nanos, timer_provider_callback_t *const cb) {
    debugf_timer("[%p] scheduled in %" PRIi64 "ns\n", this, next_time_in_nanos - get_ticks());
    event_queue->reset_timer(next_time_in_nanos, cb);
}

void timer_windows_provider_t::unschedule_oneshot() {
    debugf_timer("[%p] unscheduled\n", this);
    event_queue->unset_timer();
}

#endif  // RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_WINDOWS
