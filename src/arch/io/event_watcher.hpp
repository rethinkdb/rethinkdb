// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_IO_EVENT_WATCHER_HPP_
#define ARCH_IO_EVENT_WATCHER_HPP_

#ifdef _WIN32

#include "windows.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/interruptor.hpp"
#include "arch/runtime/runtime_utils.hpp"
#include "arch/runtime/event_queue_types.hpp"
#include "arch/io/event_watcher.hpp"
#include "errors.hpp"

class windows_event_watcher_t;

// An asynchronous operation
struct overlapped_operation_t {
    explicit overlapped_operation_t(windows_event_watcher_t *);

    // The destructor will crash if the operation is incomplete.
    ~overlapped_operation_t();

    void reset() {
        completed.reset();
    }

    // The operation was not queued or is dequeued. Mark it as
    // completed and set the result.  The IOCP event loop
    // automatically calls set_result when dequeing an
    // overlapped_operation_t.
    void set_result(size_t nb_bytes_, DWORD error_);

    // The operation was queued. Abort it, wait for it to be dequeued
    // and mark it as completed with an error.
    void abort_op();

    // The operation was not queued. Mark it as completed with an error.
    void set_cancel();

    // Wait for the operation to complete.
    void wait_interruptible(const signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);
    void wait_abortable(const signal_t *aborter);

    OVERLAPPED overlapped;           // Used by Windows to track the queued operation
    windows_event_watcher_t *event_watcher; // event_watcher for the associated handle
    size_t nb_bytes;                 // Number of bytes read or written
    DWORD error;                     // Success indicator

    signal_t *get_completed_signal() { return &completed; }

private:
    cond_t completed;                // Signaled when the operation completes or fails

    DISABLE_COPYING(overlapped_operation_t);
};

class windows_event_watcher_t {
public:
    // Assign a handle to the thread's IOCP. Should only be called once per handle.
    windows_event_watcher_t(fd_t handle, linux_event_callback_t *eh);

    // This destructor is a no-op: handle's cannot be removed from an IOCP
    ~windows_event_watcher_t();

    // TODO WINDOWS:
    // After being rethreaded, the original thread still recieves completion events
    // and it forwards them to the new thread.
    void rethread(threadnum_t new_thread);

    void stop_watching_for_errors();
    void on_error(DWORD error);
    threadnum_t current_thread() { return current_thread_; }

    const fd_t handle;

private:
    linux_event_callback_t *error_handler;
    threadnum_t original_thread;
    threadnum_t current_thread_;
};

typedef windows_event_watcher_t event_watcher_t;

#else

#include "arch/runtime/event_queue.hpp"
#include "utils.hpp"
#include "concurrency/signal.hpp"

class linux_event_watcher_t :
    public home_thread_mixin_debug_only_t,
    private linux_event_callback_t
{
public:
    linux_event_watcher_t(fd_t f, linux_event_callback_t *eh);
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
    linux_event_callback_t *error_handler;

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
