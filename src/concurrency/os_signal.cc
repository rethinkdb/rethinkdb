#include "concurrency/os_signal.hpp"
#include "arch/arch.hpp"

os_signal_monitor_t *global_os_signal_monitor = NULL;

os_signal_monitor_t& get_os_signal_monitor() {
    rassert(global_os_signal_monitor, "Trying to access the global_os_signal_monitor when it has not been set.\n");
    return *global_os_signal_monitor;
}

void set_os_signal_monitor(os_signal_monitor_t *os_signal_monitor) {
    rassert(!global_os_signal_monitor, "Trying to set global_os_signal_monitor when it was already set.\n");
    global_os_signal_monitor = os_signal_monitor;
}

os_signal_monitor_t::os_signal_monitor_t() {
    set_os_signal_monitor(this);
}

void os_signal_monitor_t::wait_for_sigint() {
    on_thread_t switcher(home_thread);
    thread_pool_t::set_interrupt_message(&sigint_pulser);
    sigint_pulser.wait();
}

bool os_signal_monitor_t::sigint_has_happened() {
    on_thread_t switcher(home_thread);
    return sigint_pulser.is_pulsed();
}

void wait_for_sigint() {
    get_os_signal_monitor().wait_for_sigint();
}

bool sigint_has_happened() {
    return get_os_signal_monitor().sigint_has_happened();
}
