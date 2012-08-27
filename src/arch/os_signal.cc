#include "arch/os_signal.hpp"
#include "arch/arch.hpp"

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
