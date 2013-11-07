// Copyright 2010-2012 RethinkDB, all rights reserved.
#define __STDC_FORMAT_MACROS
#include "arch/runtime/runtime_utils.hpp"

#include <unistd.h>

#include "arch/runtime/context_switching.hpp"
#include "arch/runtime/coroutines.hpp"
#include "logger.hpp"

#ifndef NDEBUG

#include <time.h>

uint64_t get_clock_cycles() {
#if defined(__i386__) || defined(__x86_64__)
    // uintptr_t matches the native register/word size on Linux on i386 and amd64.
    // This assumption may not be true on certain other software/hardware combinations.
    // (And of course rdtsc probably would not work.)
    uintptr_t high;
    uintptr_t low;
    __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high));
    uint64_t ret = high;
    ret <<= 32;
    ret |= low;
#else
    // Use a generic implementation using gettime, returning approximately nanoseconds
    // (1 nanosecond = 1 cycle, at 1GHz)
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    uint64_t ret = static_cast<uint64_t>(tp.tv_sec * 1e9) + static_cast<uint64_t>(tp.tv_nsec);
#endif
    return ret;
}

bool watchdog_check_enabled = false;
__thread uint64_t watchdog_start_time = 0;
const uint64_t MAX_WATCHDOG_DELTA = 100 * MILLION;
#endif  // NDEBUG

int get_cpu_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

callable_action_wrapper_t::callable_action_wrapper_t() :
    action_on_heap(false),
    action_(NULL)
{ }

callable_action_wrapper_t::~callable_action_wrapper_t()
{
    if (action_ != NULL) {
        reset();
    }
}

void callable_action_wrapper_t::reset() {
    rassert(action_ != NULL);

    if (action_on_heap) {
        delete action_;
        action_ = NULL;
        action_on_heap = false;
    } else {
        action_->~callable_action_t();
        action_ = NULL;
    }
}

void callable_action_wrapper_t::run() {
    rassert(action_ != NULL);
    action_->run_action();
}

#ifndef NDEBUG

void enable_watchdog() {
    watchdog_check_enabled = true;
}

void start_watchdog() {
    if (watchdog_check_enabled) {
        watchdog_start_time = get_clock_cycles();

        if (watchdog_start_time == 0) {
            ++watchdog_start_time;
        }
    }
}

void disarm_watchdog() {
    watchdog_start_time = 0;
}

void pet_watchdog() {
    if (watchdog_check_enabled) {
        if (watchdog_start_time == 0) {
            start_watchdog();
        } else {
            uint64_t old_value = watchdog_start_time;
            uint64_t new_value = get_clock_cycles();
            watchdog_start_time = new_value;
            uint64_t difference = new_value - old_value;
            if (difference > MAX_WATCHDOG_DELTA) {
                debugf("task triggered watchdog, elapsed cycles: %" PRIu64 ", running coroutine: %s\n", difference,
                       (coro_t::self() == NULL) ? "n/a" : coro_t::self()->get_coroutine_type().c_str());
            }
        }
    }
}

#endif  // NDEBUG

struct sigaction make_basic_sigaction() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    int res = sigfillset(&sa.sa_mask);
    guarantee_err(res == 0, "sigfillset failed");
    return sa;
}

struct sigaction make_sa_handler(int sa_flags, void (*sa_handler_func)(int)) {
    guarantee(!(sa_flags & SA_SIGINFO));
    struct sigaction sa = make_basic_sigaction();
    sa.sa_flags = sa_flags;
    sa.sa_handler = sa_handler_func;
    return sa;
}

struct sigaction make_sa_sigaction(int sa_flags, void (*sa_sigaction_func)(int, siginfo_t *, void *)) {
    guarantee(sa_flags & SA_SIGINFO);
    struct sigaction sa = make_basic_sigaction();
    sa.sa_flags = sa_flags;
    sa.sa_sigaction = sa_sigaction_func;
    return sa;
}


