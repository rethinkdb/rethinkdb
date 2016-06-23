// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_OS_SIGNAL_HPP_
#define ARCH_OS_SIGNAL_HPP_

#include "concurrency/cond_var.hpp"
#include "arch/runtime/runtime_utils.hpp"

class os_signal_cond_t : public thread_message_t,
                         public cond_t
{
public:
    os_signal_cond_t();
    ~os_signal_cond_t();
    void on_thread_switch();
    std::string format();

private:
    friend class linux_thread_pool_t;
#ifdef _WIN32
    DWORD source_type;
#else
    pid_t source_pid;
    uid_t source_uid;
    int source_signo;
#endif
};

#endif  // ARCH_OS_SIGNAL_HPP_
