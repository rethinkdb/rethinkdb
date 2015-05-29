#ifdef _WIN32

// ATN TODO

#include "arch/runtime/system_event/windows_event.hpp"
#include "arch/runtime/event_queue/windows.hpp"
#include "arch/runtime/thread_pool.hpp"

void CALLBACK on_event(ULONG_PTR cb) {
	reinterpret_cast<event_callback_t*>(cb)->on_event(poll_event_in);
}

void windows_event_t::wakey_wakey() {
	if (thread != nullptr) {
		rassert(callback != nullptr);
		QueueUserAPC(on_event, thread, reinterpret_cast<ULONG_PTR>(callback));
	}
}

#endif /* defined(_WIN32) */