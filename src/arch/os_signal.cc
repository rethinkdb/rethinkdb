// Copyright 2010-2013 RethinkDB, all rights reserved.

#include "arch/os_signal.hpp"

#include "arch/runtime/thread_pool.hpp"
#include "arch/types.hpp"
#include "do_on_thread.hpp"

os_signal_cond_t::os_signal_cond_t() :
#ifdef _WIN32
    source_type(0)
#else
    source_pid(-1),
    source_uid(-1),
    source_signo(0)
#endif
{
    DEBUG_VAR thread_message_t *old = linux_thread_pool_t::get_thread_pool()->exchange_interrupt_message(this);
    rassert(old == NULL);
}

os_signal_cond_t::~os_signal_cond_t() {
    if (!is_pulsed()) {
        DEBUG_VAR thread_message_t *old = thread_pool_t::get_thread_pool()->exchange_interrupt_message(NULL);
        rassert(old == this);
    }
}

void os_signal_cond_t::on_thread_switch() {
    // TODO: There's probably an outside chance of a use-after-free bug here.
    do_on_thread(home_thread(), std::bind(&os_signal_cond_t::pulse, this));
}

std::string os_signal_cond_t::format() {
#ifdef _WIN32
    switch (source_type) {
    case CTRL_C_EVENT:
        return "Control-C";
    case CTRL_BREAK_EVENT:
        return "Control-Break";
    case CTRL_CLOSE_EVENT:
        return "Close (End Task) Event";
    case CTRL_LOGOFF_EVENT:
        return "Logoff Event";
    case CTRL_SHUTDOWN_EVENT:
        return "Shutdown Event";
    default:
        return strprintf("console signal %ud", source_type);
    }
#else
    switch (source_signo) {
    case SIGINT:
        return strprintf("SIGINT from pid %d, uid %d",
                         source_pid, source_uid);
    case SIGTERM:
        return strprintf("SIGTERM from pid %d, uid %d",
                         source_pid, source_uid);
    default:
        return strprintf("signal %d from pid %d, uid %d",
                         source_signo, source_pid, source_uid);
    }
#endif
}
