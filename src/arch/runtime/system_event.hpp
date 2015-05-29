// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_SYSTEM_EVENT_HPP_
#define ARCH_RUNTIME_SYSTEM_EVENT_HPP_

#if defined(_WIN32)

#include "arch/runtime/system_event/windows_event.hpp"
typedef windows_event_t system_event_t;

#elif defined(NO_EVENTFD) || !defined(__linux)

#include "arch/runtime/system_event/pipe_event.hpp"
typedef pipe_event_t system_event_t;

#else

#include "arch/runtime/system_event/eventfd_event.hpp"
typedef eventfd_event_t system_event_t;

#endif

#endif // ARCH_RUNTIME_SYSTEM_EVENT_HPP_

