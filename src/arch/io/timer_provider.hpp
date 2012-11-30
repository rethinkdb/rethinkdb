// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_TIMER_PROVIDER_HPP_
#define ARCH_IO_TIMER_PROVIDER_HPP_

// We pick the right timer provider (that impelements OS level timer interface) depending on which
// system we're on. Some older kernels don't support fdtimers, so we have to resort to signals.  OS
// X doesn't support either Linux timer (shame on them) so we resort to itimer.

// These values are defined such that you can safely check that this header file was included by
// testing whether RDB_TIMER_PROVIDER has been defined, and such that RDB_TIMER_PROVIDER ==
// RDB_TIMER_PROVIDER_FOO will be false if the header file was not included.

#define RDB_TIMER_PROVIDER_TIMERFD 1
#define RDB_TIMER_PROVIDER_ITIMER 2
#define RDB_TIMER_PROVIDER_SIGNAL 3

#ifdef LEGACY_LINUX
#define RDB_TIMER_PROVIDER RDB_TIMER_PROVIDER_SIGNAL
#else
#if __MACH__
#define RDB_TIMER_PROVIDER RDB_TIMER_PROVIDER_ITIMER
#else
#define RDB_TIMER_PROVIDER RDB_TIMER_PROVIDER_TIMERFD
#endif  // __MACH__
#endif  // LEGACY_LINUX

#if RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_SIGNAL
#include "arch/io/timer/timer_signal_provider.hpp"
typedef timer_signal_provider_t timer_provider_t;
#elif RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_ITIMER
#include "arch/io/timer/timer_itimer_provider.hpp"
typedef timer_itimer_provider_t timer_provider_t;
#elif RDB_TIMER_PROVIDER == RDB_TIMER_PROVIDER_TIMERFD
#include "arch/io/timer/timerfd_provider.hpp"
typedef timerfd_provider_t timer_provider_t;
#else  // RDB_TIMER_PROVIDER == ...
#error "No timer provider define specified."
#endif  // RDB_TIMER_PROVIDER == ...

/* Timer provider callback */
struct timer_provider_callback_t {
    virtual ~timer_provider_callback_t() {}
    virtual void on_timer(int nexpirations) = 0;
};

#endif // ARCH_IO_TIMER_PROVIDER_HPP_
