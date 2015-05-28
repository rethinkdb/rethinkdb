#ifndef ARCH_RUNTIME_SYSTEM_EVENT_WINDOWS_EVENT_HPP_
#define ARCH_RUNTIME_SYSTEM_EVENT_WINDOWS_EVENT_HPP_

// ATN TODO

#include "windows.hpp"
#include "containers/scoped.hpp"

// ATN TODO: if the event is triggered before the wait, should the wait see it?

class iocp_event_t {
public:
	iocp_event_t() : id(new char), completion_port(nullptr) { }
	void wakey_wakey();
	void consume_wakey_wakeys() { }

private:
	friend class iocp_event_queue_t;
	// ATN TODO: a better way to generate unique ids
	scoped_ptr_t<char> id;
	HANDLE completion_port;

};

#endif