#include "arch/runtime/runtime_utils.hpp"

#include <unistd.h>

#include "arch/runtime/context_switching.hpp"
#include "arch/runtime/coroutines.hpp"
#include "logger.hpp"

#ifndef NDEBUG

uint64_t get_clock_cycles() {
    uint64_t ret;
    uint32_t low;
    __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (ret));
    ret <<= 32;
    ret |= low;
    return ret;
}

bool watchdog_check_enabled = false;
__thread uint64_t watchdog_start_time = 0;
const uint64_t MAX_WATCHDOG_DELTA = 100000000;
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
                debugf("task triggered watchdog, elapsed cycles: %lu, running coroutine: %s", difference,
                       (coro_t::self() == NULL) ? "n/a" : coro_t::self()->get_coroutine_type().c_str());
            }
        }
    }
}
#endif  // NDEBUG
