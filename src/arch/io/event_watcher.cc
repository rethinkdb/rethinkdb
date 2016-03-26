// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/io/event_watcher.hpp"
#include "arch/runtime/thread_pool.hpp"

#ifdef _WIN32

// TODO WINDOWS

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
