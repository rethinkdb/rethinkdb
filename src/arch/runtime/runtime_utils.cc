#include "arch/runtime/runtime_utils.hpp"
#include "arch/runtime/context_switching.hpp"
#include "arch/runtime/coroutines.hpp"
#include "logger.hpp"
#include <unistd.h>

#ifndef NDEBUG
bool watchdog_printout_enabled = false;
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

void get_clock_cycles(uint64_t *value) {
    asm volatile (
        "pushq %%rax\n\t"
        "pushq %%rbx\n\t"
        "pushq %%rdx\n\t"
        "rdtsc\n\t"
        "movq %0, %%rbx\n\t"
        "movl %%eax, (%%rbx)\n\t"
        "movl %%edx, 4(%%rbx)\n\t"
        "popq %%rdx\n\t"
        "popq %%rbx\n\t"
        "popq %%rax\n"
        : // no outputs
        : "" (value)
    );
}

// Function to check that the completion time is not too large
//  returns 0 if within the acceptable time range, or the total delta time otherwise
uint64_t get_and_check_clock_cycles(uint64_t* value, uint64_t max_delta) {
    uint64_t old_count = *value;
    get_clock_cycles(value);

    // old_count will now contain the delta
    old_count = *value - old_count;

    return (old_count < max_delta) ? 0 : old_count;
}

#ifndef NDEBUG
void enable_watchdog() {
    watchdog_printout_enabled = true;
}

void start_watchdog() {
    get_clock_cycles(&watchdog_start_time);

    if (watchdog_start_time == 0) {
        ++watchdog_start_time;
    }
}

void disarm_watchdog() {
    watchdog_start_time = 0;
}

void pet_watchdog() {
    if (watchdog_start_time == 0) {
        start_watchdog();
    } else {
        uint64_t delta = get_and_check_clock_cycles(&watchdog_start_time, MAX_WATCHDOG_DELTA);

        if (delta != 0 && watchdog_printout_enabled) {
            logWRN("task triggered watchdog, elapsed cycles: %lu, running coroutine: %s\n", delta,
                (coro_t::self() == NULL) ? "n/a" : coro_t::self()->get_coroutine_type().c_str());
        }
    }
}
#endif
