// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_OS_SIGNAL_HPP_
#define ARCH_OS_SIGNAL_HPP_

#include "concurrency/cond_var.hpp"
#include "arch/runtime/runtime_utils.hpp"

#ifdef _WIN32

// ATN TODO

class os_signal_cond_t : public cond_t {
	int get_source_signo() const;
	pid_t get_source_pid() const;
	uid_t get_source_uid() const;
};

#else

class os_signal_cond_t : public thread_message_t,
                         public cond_t
{
public:
    os_signal_cond_t();
    ~os_signal_cond_t();
    void on_thread_switch();

    // Returns the signo of the received signal
    int get_source_signo() const;

    // Returns the pid which sent the signal, used for debugging/logging
    //  (not guaranteed to be correct)
    pid_t get_source_pid() const;

    // Returns the uid of the user whose process sent the signal, used for debugging/logging
    //  (not guaranteed to be correct)
    uid_t get_source_uid() const;

private:
    friend class linux_thread_pool_t;
    pid_t source_pid;
    uid_t source_uid;
    int source_signo;
};

#endif

#endif  // ARCH_OS_SIGNAL_HPP_
