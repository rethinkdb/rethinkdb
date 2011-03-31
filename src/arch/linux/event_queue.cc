#include "event_queue.hpp"
#include <string.h>
#include "arch/linux/thread_pool.hpp"

// TODO: I guess signum is unused because we aren't interested, and
// uctx is unused because we don't have any context data.  Is this the
// true?
void event_queue_base_t::signal_handler(UNUSED int signum, siginfo_t *siginfo, UNUSED void *uctx) {
    linux_event_callback_t *callback = (linux_event_callback_t*)siginfo->si_value.sival_ptr;
    callback->on_event(siginfo->si_overrun);
}

// TODO: Why is cb unused?
void event_queue_base_t::watch_signal(const sigevent *evp, UNUSED linux_event_callback_t *cb) {
    // All events are automagically blocked by thread pool, this is a
    // typical use case for epoll_pwait/ppoll.
    
    // Establish a handler on the signal that calls the right callback
    struct sigaction sa;
    bzero((char*)&sa, sizeof(struct sigaction));
    sa.sa_sigaction = &event_queue_base_t::signal_handler;
    sa.sa_flags = SA_SIGINFO;
    
    int res = sigaction(evp->sigev_signo, &sa, NULL);
    guarantee_err(res == 0, "Could not install signal handler in event queue");
}

// TODO: Why is cb unused?
void event_queue_base_t::forget_signal(UNUSED const sigevent *evp, UNUSED linux_event_callback_t *cb) {
    // We don't support forgetting signals for now
}

/* linux_event_watcher_t */

linux_event_watcher_t::linux_event_watcher_t(fd_t f, linux_event_callback_t *c)
    : fd(f), cb(c), mask(0), registration_thread(-1) { }

linux_event_watcher_t::~linux_event_watcher_t() {
    adjust(poll_event_in|poll_event_out, 0);
}

void linux_event_watcher_t::adjust(int what_to_change, int what_to_change_it_to) {

    rassert((what_to_change & ~(poll_event_in | poll_event_out)) == 0);
    rassert((what_to_change_it_to & ~what_to_change) == 0);

    if (mask) {
        rassert(registration_thread != -1);
        rassert(registration_thread == linux_thread_pool_t::thread_id);

        int old_mask = mask;
        mask = (mask & ~what_to_change) | what_to_change_it_to;

        if (mask == 0) {
            linux_thread_pool_t::thread->queue.forget_resource(fd, cb);
            registration_thread = -1;
        } else if (mask != old_mask) {
            linux_thread_pool_t::thread->queue.adjust_resource(fd, mask, cb);
        }

    } else {
        rassert(registration_thread == -1);

        mask = (mask & ~what_to_change) | what_to_change_it_to;

        if (mask == 0) {
            // Nothing to do; it was 0 before and it's 0 now
        } else {
            linux_thread_pool_t::thread->queue.watch_resource(fd, mask, cb);
            registration_thread = linux_thread_pool_t::thread_id;
        }
    }
}

void linux_event_watcher_t::assert_thread() {
    rassert(registration_thread == -1 || registration_thread == linux_thread_pool_t::thread_id);
}
