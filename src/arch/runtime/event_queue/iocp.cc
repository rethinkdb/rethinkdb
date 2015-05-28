#if defined (_WIN32)

// TODO ATN

#include "arch/runtime/event_queue/iocp.hpp"
#include "arch/runtime/threaD_pool.hpp"

iocp_event_queue_t::iocp_event_queue_t(linux_thread_t *thread_)
	: thread(thread_) {
	completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, static_cast<ULONG_PTR>(event_type_t::OVERLAPPED_IO), 1);
	guarantee_winerr(completion_port != NULL, "CreateIoCompletionPort failed");
}

void iocp_event_queue_t::watch_event(iocp_event_t& event, event_callback_t *cb) {
	event_callbacks[reinterpret_cast<OVERLAPPED*>(event.id.get())].push_back(cb);
	if (event.completion_port != nullptr) {
		guarantee(event.completion_port == completion_port, "cannot use an event in two different event queues");
	} else {
		event.completion_port = completion_port;
	}
}

void iocp_event_queue_t::forget_event(iocp_event_t& event, event_callback_t *cb) {
	auto it = event_callbacks.find(reinterpret_cast<OVERLAPPED*>(event.id.get()));
	rassert(it != event_callbacks.end(), "Trying to forget an event callback that is not being watched");
	for (auto el = it->second.begin(); el != it->second.end(); ++el) {
		if (*el == cb) {
			it->second.erase(el);
			return;
		}
	}
	rassert(false, "Trying to forget an event callback that is not being watched");
}

void iocp_event_queue_t::run() {
	while (!thread->should_shut_down()) {
		DWORD nb_bytes;
		ULONG_PTR key;
		OVERLAPPED *overlapped;
		BOOL res = GetQueuedCompletionStatus(completion_port, &nb_bytes, &key, &overlapped, INFINITE);
		guarantee_winerr(res, "GetQueueCompletionStatus failed");
		switch (static_cast<event_type_t>(key)) {
		case event_type_t::OVERLAPPED_IO:
			// ATN TODO
			break;
		case event_type_t::EVENT_WAKEY_WAKEY:
			{
				auto it = event_callbacks.find(overlapped);
				if (it != event_callbacks.end()) {
					for (auto cb : it->second) {
						cb->on_event(poll_event_in); // ATN TODO: do users of this use the argument?
					}
					event_callbacks.erase(it);
				}
			}
			break;
		default:
			unreachable("invalid event_type_t returned by GetQueuedCompletionStatus: %d", key);
		}
	}
}

#endif