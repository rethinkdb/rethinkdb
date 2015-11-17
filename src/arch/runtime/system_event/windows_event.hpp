#ifndef ARCH_RUNTIME_SYSTEM_EVENT_WINDOWS_EVENT_HPP_
#define ARCH_RUNTIME_SYSTEM_EVENT_WINDOWS_EVENT_HPP_

#ifdef _WIN32

class linux_thread_t;
class windows_event_queue_t;

// TODO WINDOWS: implement this

class windows_event_t {
public:
    windows_event_t();
    void wakey_wakey();
    void consume_wakey_wakeys();
};

#endif

#endif
