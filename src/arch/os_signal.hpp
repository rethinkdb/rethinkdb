#ifndef __OS_SIGNAL__
#define __OS_SIGNAL__

#include "concurrency/cond_var.hpp"
#include "arch/core.hpp"

class os_signal_monitor_t : public home_thread_mixin_t {
private:
    struct sigint_pulser_t : public thread_message_t,
                             public cond_t
    {
        void on_thread_switch() { pulse(); }
    } sigint_pulser;


public:
    os_signal_monitor_t();
    void wait_for_sigint();
    bool sigint_has_happened();
};

void wait_for_sigint();
bool sigint_has_happened();

#endif
