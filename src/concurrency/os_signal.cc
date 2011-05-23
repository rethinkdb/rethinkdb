#include "concurrency/os_signal.hpp"
#include "arch/arch.hpp"

void wait_for_sigint() {
    static struct : public thread_message_t, public cond_t {
        void on_thread_switch() { pulse(); }
    } interrupt_cond;
    thread_pool_t::set_interrupt_message(&interrupt_cond);
    interrupt_cond.wait();
}
