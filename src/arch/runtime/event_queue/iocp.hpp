#ifndef ARCH_RUNTIME_EVENT_QUEUE_IOCP_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_IOCP_HPP_

#ifdef _WIN32

class linux_thread_t;
class windows_event_t;

// TODO WINDOWS: implement this

class iocp_event_queue_t {
public:
    explicit iocp_event_queue_t(linux_thread_t *);
    void watch_event(windows_event_t *, linux_event_callback_t *cb);
    void forget_event(windows_event_t *, linux_event_callback_t *cb);
    void run();
};

#endif

#endif
