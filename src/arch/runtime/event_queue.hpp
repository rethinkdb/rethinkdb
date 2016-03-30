// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_EVENT_QUEUE_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_HPP_

#include <signal.h>

#include <string>

#include "perfmon/types.hpp"
#include "arch/runtime/runtime_utils.hpp"
#include "arch/runtime/event_queue_types.hpp"

std::string format_poll_event(int event);

// Queue stats (declared here so whichever queue is chosen can access it)
// This is a singleton initialized on first use.
// Note that we cannot put it into a static variable since its initialization
// would race with the initialization of coroutine globals which are accessed
// by the constructor of `perfmon_membership_t`.
struct pm_eventloop_singleton_t {
    static perfmon_duration_sampler_t *get();
};

/* Pick the queue now*/

#if defined(_WIN32)

// Use IOCP
#include "arch/runtime/event_queue/iocp.hpp"
typedef iocp_event_queue_t linux_event_queue_t;

#elif defined(__MACH__)

// Use kqueue, which is much faster than poll on OS X
#include "arch/runtime/event_queue/kqueue.hpp"
typedef kqueue_event_queue_t linux_event_queue_t;

#elif !defined(__linux) || defined(NO_EPOLL)

// Use poll instead of epoll
#include "arch/runtime/event_queue/poll.hpp"
typedef poll_event_queue_t linux_event_queue_t;

#else

// Epoll to the rescue
#include "arch/runtime/event_queue/epoll.hpp"
typedef epoll_event_queue_t linux_event_queue_t;

#endif

#endif // ARCH_RUNTIME_EVENT_QUEUE_HPP_
