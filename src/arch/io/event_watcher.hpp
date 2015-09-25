// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_IO_EVENT_WATCHER_HPP_
#define ARCH_IO_EVENT_WATCHER_HPP_

#ifdef _WIN32

#include "windows.hpp"
#include "concurrency/cond_var.hpp"
#include "arch/runtime/runtime_utils.hpp"
#include "arch/runtime/event_queue_types.hpp"
#include "arch/io/event_watcher.hpp"

class windows_event_watcher_t;

// An asynchronous operation
struct async_operation_t {
    async_operation_t(windows_event_watcher_t *ew) : error_handler(ew) {
        debugf("ATN: %x->init async_operation_t\n", this);
        memset(&overlapped, 0, sizeof(overlapped));
    }

    void reset() { completed.reset(); }

    void set_result(size_t nb_bytes_, DWORD error_);

    OVERLAPPED overlapped;           // Used by Windows to track the queued operation
    windows_event_watcher_t *error_handler; // On error, trigger this before signaling
    cond_t completed;                 // Signaled when the operation completes or fails
    size_t nb_bytes;                 // Number of bytes read or written
    DWORD error;                     // Success indicator

private:
    DISABLE_COPYING(async_operation_t);
};

class windows_event_watcher_t {
public:
    windows_event_watcher_t(fd_t handle, event_callback_t *eh);
    ~windows_event_watcher_t();
    void stop_watching_for_errors();
    async_operation_t make_overlapped();
    void on_error(DWORD error);

private:
    event_callback_t *error_handler;
};

typedef windows_event_watcher_t event_watcher_t;

#else

#include "arch/runtime/event_queue.hpp"
#include "utils.hpp"
#include "concurrency/signal.hpp"

class linux_event_watcher_t :
    public home_thread_mixin_debug_only_t,
    private event_callback_t
{
public:
    linux_event_watcher_t(fd_t f, event_callback_t *eh);
    ~linux_event_watcher_t();

    /* To monitor for a specific event happening, instantiate `watch_t`. It will
    get pulsed the first time that the given event arrives after you create the
    `watch_t`. To wait for the event to happen again, destroy the first
    `watch_t` and create another one. */
    struct watch_t : public signal_t {
        watch_t(linux_event_watcher_t *p, int e);
        ~watch_t();
    private:
        friend class linux_event_watcher_t;
        linux_event_watcher_t *parent;
        int event;
    };

    bool is_watching(int event);

    // TODO: This is a complete hack, kill yourself out of shame for the human race.
    void stop_watching_for_errors();

private:
    fd_t fd;
    event_callback_t *error_handler;

    watch_t **get_watch_slot(int event);
    watch_t *in_watcher;
    watch_t *out_watcher;
#ifdef __linux
    watch_t *rdhup_watcher;
#endif

    bool watching_for_errors;

    int old_mask;
    bool old_watching_for_errors;
    void remask();

    void on_event(int event);

    DISABLE_COPYING(linux_event_watcher_t);
};

typedef linux_event_watcher_t event_watcher_t;

#endif // _WIN32

#endif /* ARCH_IO_EVENT_WATCHER_HPP_ */
