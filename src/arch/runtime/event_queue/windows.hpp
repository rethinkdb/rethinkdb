#ifndef ARCH_RUNTIME_EVENT_QUEUE_WINDOWS_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_WINDOWS_HPP_

// ATN TODO

#include <map>
#include <list>

#include "arch/runtime/system_event.hpp"
#include "arch/runtime/event_queue_types.hpp"
#include "arch/runtime/runtime_utils.hpp"

const int MAX_SIMULTANEOUS_EVENTS = 16;

class linux_thread_t;
class timer_provider_callback_t;

class windows_event_queue_t {
public:
    explicit windows_event_queue_t(linux_thread_t*);

    // ATN TODO: port watch_event and watch_event to other implementations
    void watch_event(windows_event_t&, event_callback_t*);
    void forget_event(windows_event_t&, event_callback_t*);

    void post_event(event_callback_t *cb);

    void add_handle(fd_t handle);

    void run();

    void set_timer(int64_t next_time_in_nanos, timer_provider_callback_t *cb);
    void unset_timer();

private:
    ticks_t next_time_in_nanos;
    timer_provider_callback_t *timer_cb;

    linux_thread_t *thread;
    HANDLE completion_port;
};

#endif
