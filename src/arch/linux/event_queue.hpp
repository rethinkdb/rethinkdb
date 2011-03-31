
#ifndef __EVENT_QUEUE_HPP__
#define __EVENT_QUEUE_HPP__

#include "perfmon.hpp"
#include "linux_utils.hpp"

// Event queue callback
struct linux_event_callback_t {
    virtual void on_event(int events) = 0;
    virtual ~linux_event_callback_t() {}
};

// Common event queue functionality
struct event_queue_base_t {
public:
    void watch_signal(const sigevent *evp, linux_event_callback_t *cb);
    void forget_signal(const sigevent *evp, linux_event_callback_t *cb);

private:
    static void signal_handler(int signum, siginfo_t *siginfo, void *uctx);
};

struct linux_queue_parent_t {
    virtual void pump() = 0;
    virtual bool should_shut_down() = 0;
    virtual ~linux_queue_parent_t() {}
};

const int poll_event_in = 1;
const int poll_event_out = 2;
const int poll_event_err = 4;
const int poll_event_hup = 8;
const int poll_event_rdhup = 16;

// Queue stats (declared here so whichever queue is chosen can access it)
extern perfmon_sampler_t pm_events_per_loop;

/* Pick the queue now*/
#ifdef NO_EPOLL

// Use poll instead of epoll
#include "arch/linux/event_queue/poll.hpp"
typedef poll_event_queue_t linux_event_queue_t;

#else

// Epoll to the rescue
#include "arch/linux/event_queue/epoll.hpp"
typedef epoll_event_queue_t linux_event_queue_t;

#endif

/*
linux_event_watcher_t is a sentry object representing registration with the
event queue.

At any time it may or may not be watching writes and may or may not be
watching reads. This is represented by its "mask", which is some bitwise
combination of poll_event_in and poll_event_out.

If it is registered for either one of writes and reads, then you also may get
poll_event_err, poll_event_hup, and poll_event_rdhup.

The callback will be called on the thread that you last called adjust() on.
It is illegal to call adjust() on a thread other than the one you last called
adjust() on unless you are registered for neither writes nor reads.
*/

struct linux_event_watcher_t {

    linux_event_watcher_t(fd_t, linux_event_callback_t*);   // Constructor initially sets mask to 0
    ~linux_event_watcher_t();

    // new_mask = (old_mask & ~what_to_change) | what_to_change_it_to;
    // (what_to_change_it_to & ~what_to_change) must be 0
    void adjust(int what_to_change, int what_to_change_it_to);

    void assert_thread();

private:
    fd_t fd;
    linux_event_callback_t *cb;
    int mask;
    int registration_thread;
    DISABLE_COPYING(linux_event_watcher_t);
};

#endif // __EVENT_QUEUE_HPP__
