#ifdef _WIN32

// ATN TODO

#include "arch/runtime/system_event/windows_event.hpp"
#include "arch/runtime/event_queue/iocp.hpp"


void iocp_event_t::wakey_wakey() {
	if (completion_port != NULL) {
		BOOL res = PostQueuedCompletionStatus(
			completion_port,
			0,
			static_cast<ULONG_PTR>(iocp_event_queue_t::event_type_t::EVENT_WAKEY_WAKEY),
			reinterpret_cast<OVERLAPPED*>(id.get()));
		guarantee_winerr(res, "PostQueuedCompletionStatus failed");
	}
}

#endif /* defined(_WIN32) */