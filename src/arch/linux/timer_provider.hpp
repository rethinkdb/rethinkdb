#ifndef __ARCH_LINUX_TIMER_PROVIDER_HPP__
#define __ARCH_LINUX_TIMER_PROVIDER_HPP__

// We pick the right timer provider (that impelements OS level timer
// interface) depending on which system we're on. Some older kernels
// don't support fdtimers, so we have to resort to signals.

#ifdef LEGACY_LINUX
#include "timer/timer_signal_provider.hpp"
typedef timer_signal_provider_t timer_provider_t;
#else
#include "timer/timerfd_provider.hpp"
typedef timerfd_provider_t timer_provider_t;
#endif

#endif // __ARCH_LINUX_TIMER_PROVIDER_HPP__
