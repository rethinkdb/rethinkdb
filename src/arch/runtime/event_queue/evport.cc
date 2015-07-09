// Copyright 2015 RethinkDB, all rights reserved.
#ifdef __sun

#include "arch/runtime/event_queue/evport.hpp"

#include <unistd.h>

#include "arch/runtime/event_queue.hpp"
#include "perfmon/perfmon.hpp"

int user_to_evport(int mode) {

    rassert((mode & (poll_event_in | poll_event_out | poll_event_rdhup)) == mode);

    int out_mode = 0;
    if (mode & poll_event_in) out_mode |= POLLIN;
    if (mode & poll_event_out) out_mode |= POLLOUT;
    if (mode & poll_event_rdhup) out_mode |= POLLRDHUP;

    return out_mode;
}

int evport_to_user(int mode) {

    rassert((mode & (POLLIN | POLLOUT | POLLERR | POLLHUP | POLLRDHUP)) == (unsigned)mode);

    int out_mode = 0;
    if (mode & POLLIN) out_mode |= poll_event_in;
    if (mode & POLLOUT) out_mode |= poll_event_out;
    if (mode & POLLERR) out_mode |= poll_event_err;
    if (mode & POLLHUP) out_mode |= poll_event_hup;
    if (mode & POLLRDHUP) out_mode |= poll_event_rdhup;

    return out_mode;
}

evport_event_queue_t::evport_event_queue_t(linux_queue_parent_t *_parent)
    : parent(_parent) {

    evport_fd = port_create();
    guarantee_err(evport_fd >= 0, "Could not create event port");
}

void evport_event_queue_t::run() {
    int res;
    uint_t i;

    while (!parent->should_shut_down()) {

        /* Block port_getn until we have at least one event */
        do {
            nevents = 1;
            res = port_getn(evport_fd, events, MAX_IO_EVENT_PROCESSING_BATCH_SIZE, &nevents, NULL);
        } while (res == -1 && get_errno() == EINTR);

        guarantee_err(res == 0, "Could not get event port");

        block_pm_duration event_loop_timer(pm_eventloop_singleton_t::get());

        for (i = 0; i < nevents; i++) {
            /*
             * forget_resource() may have amended an in-progress event which
             * has been removed, so ensure we skip it.
             */
            if (events[i].portev_user == NULL)
                continue;

            linux_event_callback_t *cb = reinterpret_cast<linux_event_callback_t *>(events[i].portev_user);
            cb->on_event(evport_to_user(events[i].portev_events));
            res = port_associate(evport_fd, PORT_SOURCE_FD, events[i].portev_object, events[i].portev_events, events[i].portev_user);
            guarantee_err(res == 0, "Could not re-associate resource");
        }

        nevents = 0;

        parent->pump();
    }
}

evport_event_queue_t::~evport_event_queue_t() {
    DEBUG_VAR int res = close(evport_fd);
    rassert_err(res == 0, "Could not close evport_fd");
}

void evport_event_queue_t::watch_resource(fd_t resource, int watch_mode, linux_event_callback_t *cb) {
    rassert(cb);

    int res = port_associate(evport_fd, PORT_SOURCE_FD, resource, user_to_evport(watch_mode), cb);
    guarantee_err(res == 0, "Could not watch resource");
}

void evport_event_queue_t::adjust_resource(fd_t resource, int watch_mode, linux_event_callback_t *cb) {
    rassert(cb);

    /*
     * Calling port_associate() with an existing resource simply updates
     * the events and callback with the new values
     */
    int res = port_associate(evport_fd, PORT_SOURCE_FD, resource, user_to_evport(watch_mode), cb);
    guarantee_err(res == 0, "Could not adjust resource");

    /* Update in-flight events. */
    for (uint_t i = 0; i < nevents; i++) {
        if (events[i].portev_user == cb) {
            events[i].portev_events = user_to_evport(watch_mode);
        }
    }
}

void evport_event_queue_t::forget_resource(fd_t resource, linux_event_callback_t *cb) {
    rassert(cb);

    /*
     * Clear the callback from any current events to ensure ::run() does not
     * execute it and so it doesn't re-associate the event while we are trying
     * to remove it.
     */
    for (uint_t i = 0; i < nevents; i++) {
        if (events[i].portev_user == cb) {
            events[i].portev_user = NULL;
        }
    }
    /*
     * We permit ENOENT as the file descriptor may have been removed by the
     * kernel or we may be currently processing it in the ::run() loop prior
     * to re-associating it.
     */
    int res = port_dissociate(evport_fd, PORT_SOURCE_FD, resource);
    guarantee_err(res == 0 || get_errno() == ENOENT, "Could not forget resource");

}

fd_t evport_event_queue_t::get_evport_fd() {
    return evport_fd;
}

#endif  // __sun
