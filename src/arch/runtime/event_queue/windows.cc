#if defined (_WIN32)

// TODO ATN

#include "arch/runtime/event_queue/windows.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "arch/io/event_watcher.hpp"

windows_event_queue_t::windows_event_queue_t(linux_thread_t *thread_)
    : thread(thread_) {
    completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
    guarantee_winerr(completion_port != NULL, "CreateIoCompletionPort failed");
}

void windows_event_queue_t::add_handle(fd_t handle) {
    completion_port = CreateIoCompletionPort(handle, completion_port, NULL, 1);
    guarantee_winerr(completion_port != NULL, "CreateIoCompletionPort failed");
}

void windows_event_queue_t::watch_event(windows_event_t& event, event_callback_t *cb) {
    rassert(event.thread == nullptr && event.callback == nullptr, "Cannot watch the same event twice");
    BOOL res = DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &event.thread, 0, false, DUPLICATE_SAME_ACCESS);
    guarantee_winerr(res, "DuplicateHandle failed");
    event.callback = cb;
    // TODO ATN: when does event get watched
}

void windows_event_queue_t::forget_event(windows_event_t& event, event_callback_t *cb) {
    if (event.thread != nullptr) {
        CloseHandle(event.thread); // TODO ATN: should this handle really be closed
        event.thread = nullptr;
    }
    event.callback = nullptr;
}

void windows_event_queue_t::run() {
    while (!thread->should_shut_down()) {
        ULONG nb_bytes;
        ULONG_PTR key;
        OVERLAPPED *overlapped;

        // TODO ATN : make sure QueueUserAPC works, if not maybe replace it with PostQueuedCompletionStatus or use GQCSEx here
        BOOL res = GetQueuedCompletionStatus(completion_port, &nb_bytes, &key, &overlapped, INFINITE);
        DWORD error = GetLastError();
        if (overlapped == NULL) {
            guarantee_xwinerr(res != 0, error, "GetQueuedCompletionStatus failed");
        }

        async_operation_t *ao = reinterpret_cast<async_operation_t*>(overlapped);
        ao->set_result(nb_bytes, error);

        thread->pump();
    }
}

#endif
