#include "arch/runtime/runtime_utils.hpp"

#include <unistd.h>

#include "arch/runtime/context_switching.hpp"
#include "arch/runtime/coroutines.hpp"
#include "logger.hpp"

#ifndef NDEBUG
// This is an inline assembly macro to get the current number of clock cycles
// Guaranteed to be super awesome
#define get_clock_cycles(res) do { \
                                  uint32_t low; \
                                  __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (res));\
                                  res = res << 32; \
                                  res |= low; \
                              } while(0)

bool watchdog_check_enabled = false;
__thread uint64_t watchdog_start_time = 0;
const uint64_t MAX_WATCHDOG_DELTA = 100000000;
#endif

int get_cpu_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

callable_action_wrapper_t::callable_action_wrapper_t() :
    action_on_heap(false),
    action_(NULL)
{ }

callable_action_wrapper_t::~callable_action_wrapper_t()
{
    if(action_ != NULL) {
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

// Function to check that the completion time is not too large
//  returns 0 if within the acceptable time range, or the total delta time otherwise
uint64_t get_and_check_clock_cycles(uint64_t& value, uint64_t max_delta) {
    uint64_t old_count = value;
    get_clock_cycles(value);

    // old_count will now contain the delta
    old_count = value - old_count;

    return (old_count < max_delta) ? 0 : old_count;
}

void enable_watchdog() {
    watchdog_check_enabled = true;
}

void start_watchdog() {
    if (watchdog_check_enabled) {
        get_clock_cycles(watchdog_start_time);

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
            uint64_t delta = get_and_check_clock_cycles(watchdog_start_time, MAX_WATCHDOG_DELTA);

            if (delta != 0) {
                debugf("task triggered watchdog, elapsed cycles: %lu, running coroutine: %s", delta,
                    (coro_t::self() == NULL) ? "n/a" : coro_t::self()->get_coroutine_type().c_str());
            }
        }
    }
}
#endif
