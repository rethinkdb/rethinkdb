
#ifndef __ARCH_LINUX_SYSTEM_EVENT_HPP__
#define __ARCH_LINUX_SYSTEM_EVENT_HPP__

#ifdef NO_EVENTFD
#include "arch/linux/event/pipe_event.hpp"
typedef pipe_event_t system_event_t;
#else
#include "arch/linux/event/eventfd_event.hpp"
typedef eventfd_event_t system_event_t;
#endif

#endif // __ARCH_LINUX_SYSTEM_EVENT_HPP__

