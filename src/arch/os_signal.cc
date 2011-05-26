#include "arch/os_signal.hpp"
#include "arch/arch.hpp"

os_signal_cond_t *global_os_signal_cond = NULL;

os_signal_cond_t& get_os_signal_cond() {
    rassert(global_os_signal_cond, "Trying to access the global_os_signal_cond when it has not been set.\n" 
                                      "Construct an os_signal_cond_t to fix this.\n");
    return *global_os_signal_cond;
}

void set_os_signal_cond(os_signal_cond_t *os_signal_cond) {
    rassert(!global_os_signal_cond, "Trying to set global_os_signal_cond when it was already set.\n");
    global_os_signal_cond = os_signal_cond;
}

os_signal_cond_t::os_signal_cond_t() {
    set_os_signal_cond(this);
    thread_pool_t::set_interrupt_message(this);
}

void wait_for_sigint() {
    on_thread_t switcher(get_os_signal_cond().get_home_thread());
    get_os_signal_cond().wait();
}
