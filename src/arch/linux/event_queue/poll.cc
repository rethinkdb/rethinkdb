
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
#include "arch/linux/event_queue/poll.hpp"
#include "arch/linux/thread_pool.hpp"

int user_to_poll(int mode) {
    int out_mode = 0;
    if(mode & poll_event_in)
        out_mode |= POLLIN;
    if(mode & poll_event_out)
        out_mode |= POLLOUT;

    return out_mode;
}

int poll_to_user(int mode) {
    int out_mode = 0;
    if(mode & POLLIN)
        out_mode |= poll_event_in;
    if(mode & POLLOUT)
        out_mode |= poll_event_out;
    if(mode & POLLRDHUP || mode & POLLERR || mode & POLLHUP)
        out_mode |= poll_event_err;

    return out_mode;
}

poll_event_queue_t::poll_event_queue_t(linux_queue_parent_t *parent)
    : parent(parent)
{
}

void poll_event_queue_t::run() {
    
    int res;
    
    // Now, start the loop
    while (!parent->should_shut_down()) {
    
        // Grab the events from the kernel!
        res = poll(&watched_fds[0], watched_fds.size(), -1);
        
        // epoll_wait might return with EINTR in some cases (in
        // particular under GDB), we just need to retry.
        if(res == -1 && errno == EINTR) {
            continue;
        }
        guarantee_err(res == 0, "Waiting for poll events failed");  // RSI
        
        pm_events_per_loop.record(res);
        
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
        
        parent->pump();
    }
}

poll_event_queue_t::~poll_event_queue_t()
{
}

void poll_event_queue_t::watch_resource(fd_t resource, int watch_mode, linux_event_callback_t *cb) {

    assert(cb);

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
            watched_fds[i].events = events;
            return;
        }
    }
}

void poll_event_queue_t::forget_resource(fd_t resource, linux_event_callback_t *cb) {

    assert(cb);

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
