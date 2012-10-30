// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_EVENT_QUEUE_HPP_
#define ARCH_RUNTIME_EVENT_QUEUE_HPP_

#include <signal.h>

#include <string>

#include "perfmon/types.hpp"
#include "arch/runtime/runtime_utils.hpp"
#include "arch/runtime/event_queue_types.hpp"

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
#include "arch/runtime/event_queue/poll.hpp"
typedef poll_event_queue_t linux_event_queue_t;

#else

// Epoll to the rescue
#include "arch/runtime/event_queue/epoll.hpp"
typedef epoll_event_queue_t linux_event_queue_t;

#endif

#endif // ARCH_RUNTIME_EVENT_QUEUE_HPP_
