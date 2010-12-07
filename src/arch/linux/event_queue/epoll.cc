
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
#include "arch/linux/io.hpp"
#include "arch/linux/event_queue/epoll.hpp"
#include "arch/linux/thread_pool.hpp"

/* Declared here but NOT in poll.cc; this instance provides for either one */
perfmon_sampler_t pm_events_per_loop("events_per_loop", secs_to_ticks(1));

int user_to_epoll(int mode) {
    int out_mode = 0;
    if(mode & poll_event_in)
        out_mode |= EPOLLIN;
    if(mode & poll_event_out)
        out_mode |= EPOLLOUT;

    return out_mode;
}

int epoll_to_user(int mode) {
    int out_mode = 0;
    if(mode & EPOLLIN)
        out_mode |= poll_event_in;
    if(mode & EPOLLOUT)
        out_mode |= poll_event_out;
    if((mode & EPOLLRDHUP) || (mode & EPOLLERR) || (mode & EPOLLHUP)) {
        out_mode |= poll_event_err;
    }

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
            continue;
        }
        guarantee_err(res != -1, "Waiting for epoll events failed");    // RSI

        // nevents might be used by forget_resource during the loop
        nevents = res;
        pm_events_per_loop.record(nevents);

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

epoll_event_queue_t::~epoll_event_queue_t()
{
    int res;
    
    res = close(epoll_fd);
    guarantee_err(res == 0, "Could not close epoll_fd");    // TODO: does this need to be changed to an assert?
}

void epoll_event_queue_t::watch_resource(fd_t resource, int watch_mode, linux_event_callback_t *cb) {

    assert(cb);
    epoll_event event;
    
    event.events = EPOLLET | user_to_epoll(watch_mode);
    event.data.ptr = (void*)cb;
    
    int res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, resource, &event);
    guarantee_err(res == 0, "Could not watch resource");
}

void epoll_event_queue_t::adjust_resource(fd_t resource, int events, linux_event_callback_t *cb) {
    epoll_event event;
    
    event.events = EPOLLET | user_to_epoll(events);
    event.data.ptr = (void*)cb;
    
    int res = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, resource, &event);
    guarantee_err(res == 0, "Could not adjust resource");
}

void epoll_event_queue_t::forget_resource(fd_t resource, linux_event_callback_t *cb) {

    assert(cb);
    
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
