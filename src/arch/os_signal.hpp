#ifndef __OS_SIGNAL__
#define __OS_SIGNAL__

#include "concurrency/cond_var.hpp"
#include "arch/core.hpp"

class os_signal_cond_t : public thread_message_t,
                            public cond_t
{
public:
    os_signal_cond_t();
    void on_thread_switch() { pulse(); }
};

void wait_for_sigint();

class sigint_indicator_t : public signal_t::waiter_t {
private:
    bool value;
    void on_signal_pulsed();

public:
    sigint_indicator_t();
    ~sigint_indicator_t();
    bool get_value();
};

#endif
