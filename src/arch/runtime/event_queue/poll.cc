#include "arch/runtime/event_queue/poll.hpp"

#include <unistd.h>
#include <sched.h>
#include <stdio.h>
#include <errno.h>
#include <poll.h>
#include <string.h>

#include <new>
#include <algorithm>
#include <string>


#include "config/args.hpp"
#include "utils.hpp"
#include "arch/runtime/event_queue.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "perfmon/perfmon.hpp"

int user_to_poll(int mode) {

    rassert((mode & (poll_event_in | poll_event_out)) == mode);

    int out_mode = 0;
    if (mode & poll_event_in) out_mode |= POLLIN;
    if (mode & poll_event_out) out_mode |= POLLOUT;
    if (mode & poll_event_rdhup) out_mode |= POLLRDHUP;

    return out_mode;
}

int poll_to_user(int mode) {

    rassert((mode & (POLLIN | POLLOUT | POLLERR | POLLHUP | POLLRDHUP)) == mode);

    int out_mode = 0;
    if (mode & POLLIN) out_mode |= poll_event_in;
    if (mode & POLLOUT) out_mode |= poll_event_out;
    if (mode & POLLERR) out_mode |= poll_event_err;
    if (mode & POLLHUP) out_mode |= poll_event_hup;
    if (mode & POLLRDHUP) out_mode |= poll_event_rdhup;

    return out_mode;
}

poll_event_queue_t::poll_event_queue_t(linux_queue_parent_t *_parent)
    : parent(_parent) {
}

void poll_event_queue_t::run() {
    int res;

#ifdef LEGACY_LINUX
    // Create a restricted sigmask for ppoll:
    // In the upcoming loop, we want to continue blocking signals
    // (especially SIGINT and SIGTERM, which the main thread
    // has interrupt handlers for). However, LEGACY_LINUX
    // builds use timerfd_provider, which requires
    // this thread to be able to catch the TIMER_NOTIFY_SIGNAL.
    // sigmask_restricted is configured to allow this
    // one signal through while blocking the other signals
    // for the main thread to handle.
    sigset_t sigmask_restricted, sigmask_full;
    res = sigfillset(&sigmask_restricted);
    guarantee_err(res == 0, "Could not create an full signal mask");
    res = sigdelset(&sigmask_restricted, TIMER_NOTIFY_SIGNAL);
    guarantee_err(res == 0, "Could not remove TIMER_NOTIFY_SIGNAL from signal mask");

    res = sigfillset(&sigmask_full);
    guarantee_err(res == 0, "Could not create a full signal mask");
#endif

    // Now, start the loop
    while (!parent->should_shut_down()) {
        // Grab the events from the kernel!
#ifdef LEGACY_LINUX
        res = ppoll(&watched_fds[0], watched_fds.size(), NULL, &sigmask_restricted);
#else
        res = poll(&watched_fds[0], watched_fds.size(), 0);
#endif

        // ppoll might return with EINTR in some cases (in particular
        // under GDB), we just need to retry.
        if (res == -1 && errno == EINTR) {
            res = 0;
        }

        // The only likely poll error here is ENOMEM, which we
        // have no way of handling, and it's probably fatal.
        guarantee_err(res != -1, "Waiting for poll events failed");

        block_pm_duration event_loop_timer(&pm_eventloop);

        int count = 0;
        for (unsigned int i = 0; i < watched_fds.size(); i++) {
            if(watched_fds[i].revents != 0) {
                linux_event_callback_t *cb = callbacks[watched_fds[i].fd];
                cb->on_event(poll_to_user(watched_fds[i].revents));
                count++;
            }
            if(count == res)
                break;
        }

#ifdef LEGACY_LINUX
        // If ppoll is busy with file descriptors, the piece of shit
        // kernel starves out signals, so we need to unblock them to
        // let the signal handlers get called, and then block them
        // right back. What a sensible fucking system.
        res = pthread_sigmask(SIG_SETMASK, &sigmask_restricted, NULL);
        guarantee_err(res == 0, "Could not unblock signals");
        res = pthread_sigmask(SIG_SETMASK, &sigmask_full, NULL);
        guarantee_err(res == 0, "Could not block signals");
#endif

        parent->pump();
    }
}

poll_event_queue_t::~poll_event_queue_t() {
}

void poll_event_queue_t::watch_resource(fd_t resource, int watch_mode, linux_event_callback_t *cb) {
    rassert(cb);

    pollfd pfd;
    bzero(&pfd, sizeof(pfd));
    pfd.fd = resource;
    pfd.events = user_to_poll(watch_mode);

    watched_fds.push_back(pfd);
    callbacks[resource] = cb;
}

void poll_event_queue_t::adjust_resource(fd_t resource, int events, linux_event_callback_t *cb) {
    // Find and adjust the event
    callbacks[resource] = cb;
    for (unsigned int i = 0; i < watched_fds.size(); i++) {
        if(watched_fds[i].fd == resource) {
            watched_fds[i].events = user_to_poll(events);
            watched_fds[i].revents &= user_to_poll(events);
            return;
        }
    }
}

void poll_event_queue_t::forget_resource(fd_t resource, DEBUG_VAR linux_event_callback_t *cb) {
    rassert(cb);

    // Erase the callback from the map
    callbacks.erase(resource);

    // Find and erase the event
    for (unsigned int i = 0; i < watched_fds.size(); i++) {
        if(watched_fds[i].fd == resource) {
            watched_fds.erase(watched_fds.begin() + i);
            return;
        }
    }

}
