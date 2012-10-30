// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/event_queue.hpp"

#include <string.h>

#include "arch/runtime/thread_pool.hpp"
#include "concurrency/cond_var.hpp"
#include "utils.hpp"
#include "perfmon/perfmon.hpp"

perfmon_duration_sampler_t pm_eventloop(secs_to_ticks(1.0));
static perfmon_membership_t pm_eventloop_membership(&get_global_perfmon_collection(), &pm_eventloop, "eventloop");

std::string format_poll_event(int event) {
    std::string s;
    if (event & poll_event_in) {
        s = "in";
    }
    if (event & poll_event_out) {
        if (s != "") s += " ";
        s += "out";
    }
    if (event & poll_event_err) {
        if (s != "") s += " ";
        s += "err";
    }
    if (event & poll_event_hup) {
        if (s != "") s += " ";
        s += "hup";
    }
    if (event & poll_event_rdhup) {
        if (s != "") s += " ";
        s += "rdhup";
    }
    if (s == "") s = "(none)";
    return s;
}

void event_queue_base_t::signal_handler(UNUSED int signum, siginfo_t *siginfo, UNUSED void *uctx) {
    linux_event_callback_t *callback = reinterpret_cast<linux_event_callback_t *>(siginfo->si_value.sival_ptr);
    callback->on_event(siginfo->si_overrun);
}

void event_queue_base_t::watch_signal(const sigevent *evp, UNUSED linux_event_callback_t *cb) {
    // All events are automagically blocked by thread pool, this is a
    // typical use case for epoll_pwait/ppoll.

    // Establish a handler on the signal that calls the right callback
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = &event_queue_base_t::signal_handler;
    sa.sa_flags = SA_SIGINFO;

    int res = sigaction(evp->sigev_signo, &sa, NULL);
    guarantee_err(res == 0, "Could not install signal handler in event queue");
}

void event_queue_base_t::forget_signal(UNUSED const sigevent *evp, UNUSED linux_event_callback_t *cb) {
    // We don't support forgetting signals for now
}

