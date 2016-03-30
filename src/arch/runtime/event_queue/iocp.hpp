#ifndef ARCH_RUNTIME_EVENT_QUEUE_IOCP_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_IOCP_HPP_

#include <map>
#include <list>

#include "arch/runtime/system_event.hpp"
#include "arch/runtime/event_queue_types.hpp"
#include "arch/runtime/runtime_utils.hpp"

class linux_thread_t;
class timer_provider_callback_t;

class iocp_event_queue_t {
public:
    explicit iocp_event_queue_t(linux_thread_t*);

    void watch_event(windows_event_t*, linux_event_callback_t*);
    void forget_event(windows_event_t*, linux_event_callback_t*);
    void post_event(linux_event_callback_t *cb);

    void add_handle(fd_t handle);

    void reset_timer(int64_t next_time_in_nanos, timer_provider_callback_t *cb);
    void unset_timer();

    void run();

private:
    ticks_t next_time_in_nanos;
    timer_provider_callback_t *timer_cb;

    linux_thread_t *thread;
    HANDLE completion_port;
};

#endif
