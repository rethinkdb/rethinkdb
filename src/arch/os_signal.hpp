#ifndef OS_SIGNAL_HPP_
#define OS_SIGNAL_HPP_

#include "concurrency/cond_var.hpp"
#include "arch/runtime/runtime.hpp"

class os_signal_cond_t : public thread_message_t,
                         public cond_t
{
public:
    os_signal_cond_t();
    ~os_signal_cond_t();
    void on_thread_switch();
};

#endif  // OS_SIGNAL_HPP_
