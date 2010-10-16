
#ifndef __POLL_EVENT_QUEUE_HPP__
#define __POLL_EVENT_QUEUE_HPP__

#include <queue>
#include <poll.h>
#include <vector>
#include "corefwd.hpp"
#include "config/args.hpp"
#include "perfmon.hpp"

// Event queue structure
struct poll_event_queue_t {
public:
    typedef std::vector<pollfd, gnew_alloc<pollfd> > pollfd_vector_t;
    typedef std::map<fd_t, linux_event_callback_t*, std::less<fd_t>,
                     gnew_alloc<std::pair<fd_t, linux_event_callback_t*> > > callback_map_t;
    
public:
    poll_event_queue_t(linux_queue_parent_t *parent);
    void run();
    ~poll_event_queue_t();

private:
    linux_queue_parent_t *parent;
    
private:
    int events_per_loop;
    perfmon_var_t<int> pm_events_per_loop;

    pollfd_vector_t watched_fds;
    callback_map_t callbacks;

public:
    // These should only be called by the event queue itself or by the linux_* classes
    void watch_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void adjust_resource(fd_t resource, int events, linux_event_callback_t *cb);
    void forget_resource(fd_t resource, linux_event_callback_t *cb);
};


#endif // __POLL_EVENT_QUEUE_HPP__
