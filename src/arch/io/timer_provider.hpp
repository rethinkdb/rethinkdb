// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_TIMER_PROVIDER_HPP_
#define ARCH_IO_TIMER_PROVIDER_HPP_

// We pick the right timer provider (that impelements OS level timer
// interface) depending on which system we're on. Some older kernels
// don't support fdtimers, so we have to resort to signals.

#ifdef LEGACY_LINUX
#define RDB_USE_TIMER_SIGNAL_PROVIDER 1
#else
#if __MACH__
#define RDB_USE_TIMER_ITIMER_PROVIDER 1
#else
#define RDB_USE_TIMERFD_PROVIDER 1
#endif  // __MACH__
#endif  // LEGACY_LINUX

#if RDB_USE_TIMER_SIGNAL_PROVIDER

#include "arch/io/timer/timer_signal_provider.hpp"
typedef timer_signal_provider_t timer_provider_t;

#elif RDB_USE_TIMER_ITIMER_PROVIDER

// TODO(OSX) Any other ways to do timing?  Just get rid of timerfd?
// TODO(OSX) timerfd is linux-specific, so timer_signal_provider_t should be in the else block.
#include "arch/io/timer/timer_itimer_provider.hpp"
typedef timer_itimer_provider_t timer_provider_t;

#elif RDB_USE_TIMERFD_PROVIDER

#include "arch/io/timer/timerfd_provider.hpp"
typedef timerfd_provider_t timer_provider_t;

#else

#error "No timer provider define specified."

#endif  // RDB_USE_TIMER_..._PROVIDER

/* Timer provider callback */
struct timer_provider_callback_t {
    virtual ~timer_provider_callback_t() {}
    virtual void on_timer(int nexpirations) = 0;
};

#endif // ARCH_IO_TIMER_PROVIDER_HPP_
