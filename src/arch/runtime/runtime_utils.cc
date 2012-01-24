#include "arch/runtime/runtime_utils.hpp"

#include <unistd.h>

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

