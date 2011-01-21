#ifndef NO_EPOLL // To make the build system happy.

#include <unistd.h>
#include <sched.h>
#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <new>
#include <algorithm>
#include <string>
#include <sstream>
#include "config/args.hpp"
#include "utils2.hpp"
#include "arch/linux/disk.hpp"
#include "arch/linux/event_queue/epoll.hpp"
#include "arch/linux/thread_pool.hpp"
#include "logger.hpp"

int user_to_epoll(int mode) {

    rassert((mode & (poll_event_in|poll_event_out)) == mode);

    int out_mode = 0;
    if (mode & poll_event_in) out_mode |= EPOLLIN;
    if (mode & poll_event_out) out_mode |= EPOLLOUT;

    return out_mode;
}

int epoll_to_user(int mode) {

    rassert((mode & (EPOLLIN|EPOLLOUT|EPOLLERR|EPOLLHUP|EPOLLRDHUP)) == mode);

    int out_mode = 0;
    if (mode & EPOLLIN) out_mode |= poll_event_in;
    if (mode & EPOLLOUT) out_mode |= poll_event_out;
    if (mode & EPOLLERR) out_mode |= poll_event_err;
    if (mode & EPOLLHUP) out_mode |= poll_event_hup;
    if (mode & EPOLLRDHUP) out_mode |= poll_event_rdhup;

    return out_mode;
}

epoll_event_queue_t::epoll_event_queue_t(linux_queue_parent_t *parent)
    : parent(parent)
{
    // Create a poll fd
    
    epoll_fd = epoll_create1(0);
    guarantee_err(epoll_fd >= 0, "Could not create epoll fd");
}

void epoll_event_queue_t::run() {
    int res;
    
    // Now, start the loop
    while (!parent->should_shut_down()) {
        // Grab the events from the kernel!
        res = epoll_wait(epoll_fd, events, MAX_IO_EVENT_PROCESSING_BATCH_SIZE, -1);
        
        // epoll_wait might return with EINTR in some cases (in
        // particular under GDB), we just need to retry.
        if(res == -1 && errno == EINTR) {
            // If the thread has been signalled, we already handled
            // the signal in our signal action.
            res = 0;
        }

        // When epoll_wait returns with EBADF, EFAULT, and EINVAL,
        // it probably means that epoll_fd is no longer valid. There's
        // no reason to try epoll_wait again, unless we can create a
        // new descriptor (which we probably can't at this point). 
        guarantee_err(res != -1, "Waiting for epoll events failed");

        // nevents might be used by forget_resource during the loop
        nevents = res;

        // TODO: instead of processing the events immediately, we
        // might want to queue them up and then process the queue in
        // bursts. This might reduce response time but increase
        // overall throughput because it will minimize cache faults
        // associated with instructions as well as data structures
        // (see section 7 [CPU scheduling] in B-tree Indexes and CPU
        // Caches by Goetz Graege and Pre-Ake Larson).

        for (int i = 0; i < nevents; i++) {
            if (events[i].data.ptr == NULL) {
                // The event was queued for a resource that's
                // been destroyed, so forget_resource is kindly
                // notifying us to skip it.
                continue;
            } else {
                linux_event_callback_t *cb = (linux_event_callback_t*)events[i].data.ptr;
                cb->on_event(epoll_to_user(events[i].events));
            }
        }

        nevents = 0;
        
        parent->pump();
    }
}

epoll_event_queue_t::~epoll_event_queue_t() {
    int res;
    
    res = close(epoll_fd);
    rassert_err(res == 0, "Could not close epoll_fd");
}

void epoll_event_queue_t::watch_resource(fd_t resource, int watch_mode, linux_event_callback_t *cb) {
    rassert(cb);
    epoll_event event;
    
    event.events = EPOLLET | user_to_epoll(watch_mode);
    event.data.ptr = (void*)cb;
    
    int res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, resource, &event);
    guarantee_err(res == 0, "Could not watch resource");
}

void epoll_event_queue_t::adjust_resource(fd_t resource, int watch_mode, linux_event_callback_t *cb) {
    epoll_event event;
    
    event.events = EPOLLET | user_to_epoll(watch_mode);
    event.data.ptr = (void*)cb;
    
    int res = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, resource, &event);
    guarantee_err(res == 0, "Could not adjust resource");

    // Go through the queue of messages in the current poll cycle and if any are referring
    // to the resource we are adjusting, remove any events that we are no longer requesting.
    for (int i = 0; i < nevents; i++) {
        if (events[i].data.ptr == (void*)cb) {
            events[i].events &= EPOLLET | user_to_epoll(watch_mode);
        }
    }
}

void epoll_event_queue_t::forget_resource(fd_t resource, linux_event_callback_t *cb) {
    rassert(cb);
    
    epoll_event event;
    
    event.events = EPOLLIN;
    event.data.ptr = NULL;
    
    int res = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, resource, &event);
    guarantee_err(res == 0, "Couldn't remove resource from watching");

    // Go through the queue of messages in the current poll cycle and
    // clean out the ones that are referencing the resource we're
    // being asked to forget.
    for (int i = 0; i < nevents; i++) {
        if (events[i].data.ptr == (void*)cb) {
            events[i].data.ptr = NULL;
        }
    }
}

#endif
