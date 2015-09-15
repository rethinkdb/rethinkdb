#ifdef _WIN32

// ATN TODO

#include "arch/runtime/system_event/windows_event.hpp"
#include "arch/runtime/event_queue/windows.hpp"
#include "arch/runtime/thread_pool.hpp"

void windows_event_t::wakey_wakey() {
    if (completion_port != INVALID_HANDLE_VALUE) {
        rassert(callback != nullptr);
        PostQueuedCompletionStatus(completion_port, 0, ULONG_PTR(windows_message_type_t::EVENT), reinterpret_cast<OVERLAPPED*>(this));
    }
}

#endif /* defined(_WIN32) */
