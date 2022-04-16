// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/event_queue.hpp"

#include <string.h>

#include "arch/runtime/thread_pool.hpp"
#include "concurrency/cond_var.hpp"
#include "utils.hpp"
#include "perfmon/perfmon.hpp"

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
#ifdef __linux
    if (event & poll_event_rdhup) {
        if (s != "") s += " ";
        s += "rdhup";
    }
#endif
    if (s == "") s = "(none)";
    return s;
}
