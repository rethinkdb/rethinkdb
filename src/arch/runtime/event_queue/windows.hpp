#ifndef ARCH_RUNTIME_EVENT_QUEUE_WINDOWS_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_WINDOWS_HPP_

// ATN TODO

#include <map>
#include <list>

#include "arch/runtime/system_event.hpp"
#include "arch/runtime/event_queue_types.hpp"

class linux_thread_t;

class windows_event_queue_t {
public:
	explicit windows_event_queue_t(linux_thread_t*);

	// ATN TODO: port watch_event and watch_event to other implementations
	void watch_event(windows_event_t&, event_callback_t*);
	void forget_event(windows_event_t&, event_callback_t*);
	void run();

private:
	linux_thread_t *thread;
};

#endif