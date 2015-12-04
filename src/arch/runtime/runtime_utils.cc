// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/runtime_utils.hpp"

#include <unistd.h>

#include "arch/runtime/context_switching.hpp"
#include "arch/runtime/coroutines.hpp"
#include "logger.hpp"

int get_cpu_count() {
#ifndef _WIN32
    return sysconf(_SC_NPROCESSORS_ONLN);
#else
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#endif
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

#ifndef _WIN32

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

#endif

