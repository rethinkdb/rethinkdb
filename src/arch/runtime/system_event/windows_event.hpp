#ifndef ARCH_RUNTIME_SYSTEM_EVENT_WINDOWS_EVENT_HPP_
#define ARCH_RUNTIME_SYSTEM_EVENT_WINDOWS_EVENT_HPP_

#include "windows.hpp"
#include "containers/scoped.hpp"
#include "arch/runtime/event_queue_types.hpp"

class linux_thread_t;
class iocp_event_queue_t;

class windows_event_t {
public:
    windows_event_t() : event_queue(nullptr), callback(nullptr) { }
    void wakey_wakey();
    void consume_wakey_wakeys() { }

private:
    friend class iocp_event_queue_t;

    iocp_event_queue_t *event_queue;
    linux_event_callback_t *callback;
};

#endif
