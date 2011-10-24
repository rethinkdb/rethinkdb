
#ifndef __ARCH_RUNTIME_SYSTEM_EVENT_HPP__
#define __ARCH_RUNTIME_SYSTEM_EVENT_HPP__

#ifdef NO_EVENTFD
#include "arch/runtime/system_event/pipe_event.hpp"
typedef pipe_event_t system_event_t;
#else
#include "arch/runtime/system_event/eventfd_event.hpp"
typedef eventfd_event_t system_event_t;
#endif

#endif // __ARCH_RUNTIME_SYSTEM_EVENT_HPP__

