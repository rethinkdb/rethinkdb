// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/event_queue.hpp"

#include <string.h>

#include "arch/runtime/thread_pool.hpp"
#include "concurrency/cond_var.hpp"
#include "utils.hpp"
#include "perfmon/perfmon.hpp"

perfmon_duration_sampler_t *pm_eventloop_singleton_t::get() {
    static perfmon_duration_sampler_t pm_eventloop(secs_to_ticks(1));
    static perfmon_membership_t pm_eventloop_membership(
        &get_global_perfmon_collection(), &pm_eventloop, "eventloop");
    return &pm_eventloop;
}

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
#if defined(__linux) || defined(__sun)
    if (event & poll_event_rdhup) {
        if (s != "") s += " ";
        s += "rdhup";
    }
#endif
    if (s == "") s = "(none)";
    return s;
}
