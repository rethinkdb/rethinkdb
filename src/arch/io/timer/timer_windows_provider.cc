// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/io/timer_provider.hpp"

#if RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_WINDOWS

#include <inttypes.h>

#include "logger.hpp"
#include "time.hpp"

// TODO ATN

/*
#define debugf_timer(...) debugf("ATN: timer: " __VA_ARGS__) /*/
#define debugf_timer(...) ((void)0) //*/

const HANDLE DEFAULT_TIMER_QUEUE = nullptr;

timer_windows_provider_t::timer_windows_provider_t(iocp_event_queue_t *event_queue_) :
    callback(nullptr), event_queue(event_queue_) {
    debugf_timer("[%p] create\n", this);
    // ATN TODO: assert that there is no other timer provider for this thread
}

timer_windows_provider_t::~timer_windows_provider_t() {
    debugf_timer("[%p] destroy\n", this);
    unschedule_oneshot();
}

void timer_windows_provider_t::on_event(UNUSED int) {
    debugf_timer("[%p] callback in original thread\n", this);
    crash("ATN TODO");
}

void timer_windows_provider_t::schedule_oneshot(const int64_t next_time_in_nanos, timer_provider_callback_t *const cb) {
    debugf_timer("[%p] scheduled in %" PRIi64 "ns\n", this, next_time_in_nanos - get_ticks());
    event_queue->set_timer(next_time_in_nanos, cb);
}

void timer_windows_provider_t::unschedule_oneshot() {
    debugf_timer("[%p] unscheduled\n", this);
    event_queue->unset_timer();
}

#endif  // RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_WINDOWS
