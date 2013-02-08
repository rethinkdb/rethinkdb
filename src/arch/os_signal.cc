// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/os_signal.hpp"

#include "arch/runtime/thread_pool.hpp"
#include "arch/types.hpp"

os_signal_cond_t::os_signal_cond_t() {
    DEBUG_VAR thread_message_t *old = thread_pool_t::set_interrupt_message(this);
    rassert(old == NULL);
}

os_signal_cond_t::~os_signal_cond_t() {
    if (!is_pulsed()) {
        DEBUG_VAR thread_message_t *old = thread_pool_t::set_interrupt_message(NULL);
        rassert(old == this);
    }
}

void os_signal_cond_t::on_thread_switch() {
    pulse();
}
