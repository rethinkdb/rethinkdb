#ifndef ARCH_RUNTIME_EVENT_QUEUE_WINDOWS_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_WINDOWS_HPP_

// ATN TODO

#include <map>
#include <list>

#include "arch/runtime/system_event.hpp"
#include "arch/runtime/event_queue_types.hpp"

class linux_thread_t;

class iocp_event_queue_t {
public:
	explicit iocp_event_queue_t(linux_thread_t*);

	// ATN TODO: port watch_event to other implementations
	void watch_event(iocp_event_t&, event_callback_t*);
	void forget_event(iocp_event_t&, event_callback_t*);
	void run();

	enum class event_type_t {
		OVERLAPPED_IO,
		EVENT_WAKEY_WAKEY
	};

private:
	linux_thread_t *thread;
	HANDLE completion_port;
	std::map<OVERLAPPED*, std::list<event_callback_t*>> event_callbacks;
};

#endif