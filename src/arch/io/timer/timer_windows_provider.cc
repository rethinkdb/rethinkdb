// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/io/timer_provider.hpp"

#if RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_WINDOWS

#include "logger.hpp"
#include "time.hpp"

// TODO ATN

timer_windows_provider_t::timer_windows_provider_t(UNUSED event_queue_t *) {
	timer = CreateWaitableTimer(nullptr, false, nullptr);
	guarantee_winerr(timer != nullptr, "CreateWaitableTimer failed");
}

timer_windows_provider_t::~timer_windows_provider_t() {
	CloseHandle(timer);
}

void CALLBACK on_oneshot(void *cb, UNUSED DWORD, UNUSED DWORD) {
	reinterpret_cast<timer_provider_callback_t*>(cb)->on_oneshot();
}

void timer_windows_provider_t::schedule_oneshot(const int64_t next_time_in_nanos, timer_provider_callback_t *const cb) {
	LARGE_INTEGER due_time;
	due_time.QuadPart = next_time_in_nanos / 100; // ATN TODO maybe convert to relative time?
	BOOL res = SetWaitableTimer(timer, &due_time, 0, on_oneshot, cb, false);
	guarantee_winerr(res, "SetWaitableTimer failed");
}

void timer_windows_provider_t::unschedule_oneshot() {
	BOOL res = CancelWaitableTimer(timer);
	guarantee_winerr(res, "CancelWaitableTimer failed");
}

#endif  // RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_WINDOWS
