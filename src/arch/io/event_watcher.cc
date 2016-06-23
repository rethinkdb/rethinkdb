// Copyright 2010-2013 RethinkDB, all rights reserved.

#include "arch/io/event_watcher.hpp"
#include "arch/runtime/thread_pool.hpp"

#ifdef _WIN32

#include "concurrency/wait_any.hpp"

#ifdef TRACE_OVERLAPPED
#include "debug.hpp"
#define debugf_overlapped(...) debugf("overlapped: " __VA_ARGS__)
#else
#define debugf_overlapped(...) ((void)0)
#endif

overlapped_operation_t::overlapped_operation_t(windows_event_watcher_t *ew) : event_watcher(ew) {
    debugf_overlapped("[%p] init from watcher %p\n", this, ew);
    rassert(event_watcher != nullptr);
    rassert(event_watcher->current_thread() == get_thread_id());
    memset(&overlapped, 0, sizeof(overlapped));
}

overlapped_operation_t::~overlapped_operation_t() {
    debugf_overlapped("[%p] destroy\n", this);
    // Always call wait_abortable, set_cancel, set_result or abort_op before destructing
    rassert(completed.is_pulsed());
}

void overlapped_operation_t::set_cancel() {
    debugf_overlapped("[%p] set_cancel\n", this);
    set_result(0, ERROR_CANCELLED);
}

void overlapped_operation_t::abort_op() {
    if (completed.is_pulsed()) {
        debugf_overlapped("[%p] abort_op: already pulsed\n", this);
    } else {
        debugf_overlapped("[%p] abort_op: cancelling\n", this);
        BOOL res = CancelIoEx(event_watcher->handle, &overlapped);
        if (!res) {
            switch (GetLastError()) {
            case ERROR_NOT_FOUND:
            case ERROR_INVALID_HANDLE:
                debugf_overlapped("[%p] abort_op: not found or invalid\n", this);
                // Assume the operation has already completed asynchonously and is
                // currently queued on the completion port
                break;
            default:
                guarantee_winerr(res, "CancelIoEx failed");
            }
        }

        // After a successful CancelIoEx, the operation gets queued
        // onto the completion port. In most cases the error code will
        // be ERROR_OPERATION_ABORTED. However the operation might
        // also complete succesfully or fail with another error.
        completed.wait_lazily_unordered();
    }
}

void overlapped_operation_t::set_result(size_t nb_bytes_, DWORD error_) {
    debugf_overlapped("[%p] set_result(%zu, %u)\n", this, nb_bytes_, error_);
    rassert(!completed.is_pulsed());
    nb_bytes = nb_bytes_;
    error = error_;
    if (error != NO_ERROR) {
        event_watcher->on_error(error);
    }
    completed.pulse();
}

windows_event_watcher_t::windows_event_watcher_t(fd_t handle_, linux_event_callback_t *eh) :
    handle(handle_), error_handler(eh), original_thread(get_thread_id()), current_thread_(get_thread_id()) {
    linux_thread_pool_t::get_thread()->queue.add_handle(handle);
}

void windows_event_watcher_t::rethread(threadnum_t new_thread) {
    current_thread_ = new_thread;
}

void windows_event_watcher_t::stop_watching_for_errors() {
    error_handler = nullptr;
}

void windows_event_watcher_t::on_error(UNUSED DWORD error) {
    rassert(current_thread_ == get_thread_id());
    if (error_handler != nullptr) {
        linux_event_callback_t *eh = error_handler;
        error_handler = nullptr;
        eh->on_event(poll_event_err);
    }
}

windows_event_watcher_t::~windows_event_watcher_t() {
    // WINDOWS TODO: Is there a way to make sure no more operations are
    // queued for this handle, or that the handle is closed?
}

void overlapped_operation_t::wait_interruptible(const signal_t *interruptor) {
    debugf_overlapped("[%p] wait_interruptible\n", this);
    try {
        ::wait_interruptible(&completed, interruptor);
    } catch (const interrupted_exc_t &) {
        debugf_overlapped("[%p] interrupted\n", this);
        abort_op();
        throw;
    }
}

void overlapped_operation_t::wait_abortable(const signal_t *aborter) {
    debugf_overlapped("[%p] wait_abortable\n", this);
    wait_any_t waiter(&completed, aborter);
    waiter.wait_lazily_unordered();
    if (aborter->is_pulsed()) {
        debugf_overlapped("[%p] aborted\n", this);
        abort_op();
    }
}

#else

linux_event_watcher_t::linux_event_watcher_t(fd_t f, linux_event_callback_t *eh) :
    fd(f), error_handler(eh),
    in_watcher(nullptr), out_watcher(nullptr),
#ifdef __linux
    rdhup_watcher(nullptr),
#endif
    watching_for_errors(true),
    old_mask(0),
    old_watching_for_errors(false)
{
    /* At first, only register for error events */
    remask();
}

linux_event_watcher_t::~linux_event_watcher_t() {
    guarantee(!in_watcher);
    guarantee(!out_watcher);
#ifdef __linux
    guarantee(!rdhup_watcher);
#endif

    stop_watching_for_errors();
}

void linux_event_watcher_t::stop_watching_for_errors() {
    if (watching_for_errors) {
        watching_for_errors = false;
        remask();
    }
}

linux_event_watcher_t::watch_t::watch_t(linux_event_watcher_t *p, int e) :
    parent(p), event(e)
{
    rassert(!*parent->get_watch_slot(event), "something's already watching that event.");
    *parent->get_watch_slot(event) = this;
    parent->remask();
}

linux_event_watcher_t::watch_t::~watch_t() {
    rassert(*parent->get_watch_slot(event) == this);
    *parent->get_watch_slot(event) = nullptr;
    parent->remask();
}

bool linux_event_watcher_t::is_watching(int event) {
    assert_thread();
    return *get_watch_slot(event) == nullptr;
}

linux_event_watcher_t::watch_t **linux_event_watcher_t::get_watch_slot(int event) {
    switch (event) {
    case poll_event_in:    return &in_watcher;
    case poll_event_out:   return &out_watcher;
#ifdef __linux
    case poll_event_rdhup: return &rdhup_watcher;
#endif
    default: crash("bad event");
    }
}

void linux_event_watcher_t::remask() {
    int new_mask = 0;
    if (in_watcher)    new_mask |= poll_event_in;
    if (out_watcher)   new_mask |= poll_event_out;
#ifdef __linux
    if (rdhup_watcher) new_mask |= poll_event_rdhup;
#endif

    // What we do (watch_resource, adjust_resource, forget_resource) depends on whether we are
    // currently registered to watch the resource.

    const bool old_registered = (old_mask != 0 || old_watching_for_errors);
    const bool new_registered = (new_mask != 0 || watching_for_errors);

    if (old_registered) {
        if (new_registered) {
            if (old_mask != new_mask) {
                linux_thread_pool_t::get_thread()->queue.adjust_resource(fd, new_mask, this);
            }
        } else {
            linux_thread_pool_t::get_thread()->queue.forget_resource(fd, this);
        }
    } else {
        if (new_registered) {
            linux_thread_pool_t::get_thread()->queue.watch_resource(fd, new_mask, this);
        }
    }

    old_watching_for_errors = watching_for_errors;
    old_mask = new_mask;
}

void linux_event_watcher_t::on_event(int event) {
    int error_mask = poll_event_err | poll_event_hup;
#ifdef __linux
    error_mask |= poll_event_rdhup;
#endif
    guarantee((event & (error_mask | old_mask)) == event, "Unexpected event received (from operating system?).");

    if (event & error_mask) {
#ifdef __linux
        if (event & ~poll_event_rdhup) {
            error_handler->on_event(event & error_mask);
        } else {
            rassert(event & poll_event_rdhup);
            if (!rdhup_watcher->is_pulsed()) rdhup_watcher->pulse();
        }
#else
        error_handler->on_event(event & error_mask);
#endif  // __linux
    }

    // An error condition could cause spurious wakeups of in and out watchers,
    // but that's okay because they're supposed to be able to handle spurious
    // wakeups.  (They'll just get EAGAIN.)

    if ((event & poll_event_in) || (event & error_mask)) {
        if (in_watcher != nullptr && !in_watcher->is_pulsed()) {
            in_watcher->pulse();
        }
    }

    if ((event & poll_event_out) || (event & error_mask)) {
        if (out_watcher != nullptr && !out_watcher->is_pulsed()) {
            out_watcher->pulse();
        }
    }
}

#endif
