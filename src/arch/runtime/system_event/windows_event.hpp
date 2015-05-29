#ifndef ARCH_RUNTIME_SYSTEM_EVENT_WINDOWS_EVENT_HPP_
#define ARCH_RUNTIME_SYSTEM_EVENT_WINDOWS_EVENT_HPP_

// ATN TODO

#include "windows.hpp"
#include "containers/scoped.hpp"
#include "arch/runtime/event_queue_types.hpp"

class linux_thread_t;

// ATN TODO: if the event is triggered before the wait, should the wait see it?

class windows_event_t {
public:
	windows_event_t() : thread(nullptr), callback(nullptr) { }
	~windows_event_t() {
		if (thread != nullptr) {
			CloseHandle(thread);
		}
	}
	void wakey_wakey();
	void consume_wakey_wakeys() { }

private:
	friend class windows_event_queue_t;
	HANDLE thread;
	event_callback_t *callback;
};

#endif