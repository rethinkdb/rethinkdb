#include "concurrency/os_signal.hpp"
#include "arch/arch.hpp"

struct sigint_pulser_t : public thread_message_t,
                      public cond_t
{
    void on_thread_switch() { pulse(); }
};

sigint_pulser_t& get_sigint_pulser() {
    static sigint_pulser_t sigint_pulser;
    return sigint_pulser;
}

void wait_for_sigint() {
    thread_pool_t::set_interrupt_message(&get_sigint_pulser());
    get_sigint_pulser().wait();
}

bool sigint_has_happened() {
    return get_sigint_pulser().is_pulsed();
}
