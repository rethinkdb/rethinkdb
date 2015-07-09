// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_IO_EVENT_WATCHER_HPP_
#define ARCH_IO_EVENT_WATCHER_HPP_

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
#if defined(__linux) || defined(__sun)
    watch_t *rdhup_watcher;
#endif

    bool watching_for_errors;

    int old_mask;
    bool old_watching_for_errors;
    void remask();

    void on_event(int event);

    DISABLE_COPYING(linux_event_watcher_t);
};

#endif /* ARCH_IO_EVENT_WATCHER_HPP_ */
