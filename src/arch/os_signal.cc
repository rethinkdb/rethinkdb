// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef _WIN32

#include "arch/os_signal.hpp"

#include "arch/runtime/thread_pool.hpp"
#include "arch/types.hpp"
#include "do_on_thread.hpp"

os_signal_cond_t::os_signal_cond_t() :
    source_pid(-1),
    source_uid(-1),
    source_signo(0)
{
    DEBUG_VAR thread_message_t *old = thread_pool_t::set_interrupt_message(this);
    rassert(old == NULL);
}

os_signal_cond_t::~os_signal_cond_t() {
    if (!is_pulsed()) {
        DEBUG_VAR thread_message_t *old = thread_pool_t::set_interrupt_message(NULL);
        rassert(old == this);
    }
}

int os_signal_cond_t::get_source_signo() const {
    return source_signo;
}

pid_t os_signal_cond_t::get_source_pid() const {
    return source_pid;
}

uid_t os_signal_cond_t::get_source_uid() const {
    return source_uid;
}

void os_signal_cond_t::on_thread_switch() {
    // TODO: There's probably an outside chance of a use-after-free bug here.
    do_on_thread(home_thread(), std::bind(&os_signal_cond_t::pulse, this));
}

#endif // !defined(_WIN32)
