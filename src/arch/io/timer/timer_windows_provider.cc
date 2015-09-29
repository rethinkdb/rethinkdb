// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/io/timer_provider.hpp"

#if RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_WINDOWS

#include <inttypes.h>

#include "logger.hpp"
#include "time.hpp"

// TODO ATN

/*
#define debugf_timer debugf /*/
#define debugf_timer(...) ((void)0) //*/

const HANDLE DEFAULT_TIMER_QUEUE = nullptr;

timer_windows_provider_t::timer_windows_provider_t(windows_event_queue_t *event_queue_) :
    event_queue(event_queue_), callback(nullptr), timer(nullptr) {
    debugf_timer("[%p] create\n", this);
}

timer_windows_provider_t::~timer_windows_provider_t() {
    debugf_timer("[%p] destroy\n", this);
    if (timer != nullptr) {
        guarantee_winerr(DeleteTimerQueueTimer(nullptr, timer, nullptr));
    }
}

void CALLBACK windows_timer_callback(void *data, UNUSED BOOLEAN wait_or_fired) {
    auto tp = reinterpret_cast<timer_windows_provider_t*>(data);
    tp->on_oneshot();
}

void timer_windows_provider_t::on_oneshot() {
    debugf_timer("[%p] callback in timer thread\n", this);
    event_queue->post_event(this);
}

void timer_windows_provider_t::on_event(UNUSED int) {
    debugf_timer("[%p] callback in original thread\n", this);
    rassert(callback != nullptr);
    callback->on_oneshot();
}

void timer_windows_provider_t::schedule_oneshot(const int64_t next_time_in_nanos, timer_provider_callback_t *const cb) {
    debugf_timer("[%p] scheduled in %" PRIi64 "ns\n", this, next_time_in_nanos - get_ticks());
    if (timer != nullptr) {
        // TODO ATN: possible race condition
        BOOL res = DeleteTimerQueueTimer(nullptr, timer, nullptr);
        guarantee_winerr(res || GetLastError() == ERROR_IO_PENDING, "DeleteTimerQueueTimer failed");
    }
    callback = cb;
    int64_t srelative_ms = (next_time_in_nanos - get_ticks()) / MILLION;
    DWORD urelative_ms = srelative_ms <= 0 ? 1 : srelative_ms;
    BOOL res = CreateTimerQueueTimer(&timer, DEFAULT_TIMER_QUEUE, windows_timer_callback, this, urelative_ms, 0, WT_EXECUTEINTIMERTHREAD);
    guarantee_winerr(res, "CreateTimerQueueTimer failed");
}

void timer_windows_provider_t::unschedule_oneshot() {
    debugf_timer("[%p] unscheduled\n", this);
    if (timer != nullptr) {
        // TODO ATN: possible race condition
        BOOL res = DeleteTimerQueueTimer(nullptr, timer, NULL);
        if (!res) {
            DWORD error = GetLastError();
            guarantee_xwinerr(error != ERROR_IO_PENDING, error, "DeleteTimerQueueTimer failed");
        }
        timer = nullptr;
    }
}

#endif  // RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_WINDOWS
