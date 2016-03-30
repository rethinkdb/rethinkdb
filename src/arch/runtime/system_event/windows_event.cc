#ifdef _WIN32

#include "arch/runtime/system_event/windows_event.hpp"

#include "arch/runtime/event_queue/iocp.hpp"
#include "arch/runtime/thread_pool.hpp"

void windows_event_t::wakey_wakey() {
    if (event_queue != nullptr) {
        rassert(callback != nullptr);
        event_queue->post_event(callback);
    }
}

#endif
