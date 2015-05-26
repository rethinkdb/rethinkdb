#ifndef ARCH_RUNTIME_EVENT_QUEUE_WINDOWS_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_WINDOWS_HPP_

// ATN TODO

class linux_thread_t;

class windows_event_queue_t {
public:
	explicit windows_event_queue_t(linux_thread_t*);

	void run();
	void watch_resource(int, int, linux_thread_t*);
	void watch_resource(int, int, linux_message_hub_t*);
};

#endif