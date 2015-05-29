#if defined (_WIN32)

// TODO ATN

#include "arch/runtime/event_queue/windows.hpp"
#include "arch/runtime/thread_pool.hpp"

windows_event_queue_t::windows_event_queue_t(linux_thread_t *thread_)
	: thread(thread_) { }

void windows_event_queue_t::watch_event(windows_event_t& event, event_callback_t *cb) {
	rassert(event.thread == nullptr && event.callback == nullptr, "Cannot watch the same event twice");
	BOOL res = DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &event.thread, 0, false, DUPLICATE_SAME_ACCESS);
	guarantee_winerr(res, "DuplicateHandle failed");
	event.callback = cb;
}

void windows_event_queue_t::forget_event(windows_event_t& event, event_callback_t *cb) {
	event.thread = nullptr;
	event.callback = nullptr;
}

void windows_event_queue_t::run() {
	while (!thread->should_shut_down()) {
		SleepEx(INFINITE, true);
	}
}

#endif