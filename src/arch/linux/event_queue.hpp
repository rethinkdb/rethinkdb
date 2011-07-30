
#ifndef __EVENT_QUEUE_HPP__
#define __EVENT_QUEUE_HPP__

#include <signal.h>

#include "errors.hpp"
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>

#include "perfmon_types.hpp"
#include "arch/linux/linux_utils.hpp"
#include "arch/linux/event_queue_types.hpp"

const int poll_event_in = 1;
const int poll_event_out = 2;
const int poll_event_err = 4;
const int poll_event_hup = 8;
const int poll_event_rdhup = 16;

std::string format_poll_event(int event);

// Queue stats (declared here so whichever queue is chosen can access it)
extern perfmon_duration_sampler_t pm_eventloop;

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

#endif // __EVENT_QUEUE_HPP__
