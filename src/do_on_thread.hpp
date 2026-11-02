// Copyright 2010-2013 RethinkDB, all rights reserved.

/**
 * @file do_on_thread.hpp
 * @brief Convenient API for executing callables on specific threads.
 *
 * Provides a higher-level interface than thread_message_t for executing
 * function objects on different threads without requiring subclassing.
 */

#ifndef DO_ON_THREAD_HPP_
#define DO_ON_THREAD_HPP_

#include "arch/runtime/runtime.hpp"
#include "utils.hpp"

/**
 * @brief FSM for executing a callable on one thread and returning home.
 *
 * Internal state machine that executes a callable on a target thread, then
 * returns to the home thread to clean up (for libtcmalloc affinity).
 *
 * @tparam callable_t Type of the callable object to execute.
 *
 * Usage (internal):
 * @code
 * // Do not use directly; use do_on_thread() instead
 * @endcode
 */
template <class callable_t>
struct thread_doer_t : public thread_message_t, public home_thread_mixin_t {
    const callable_t callable;
    threadnum_t thread;
    enum state_t {
        state_go_to_core,
        state_go_home
    } state;

    thread_doer_t(callable_t &&_callable, threadnum_t _thread)
        : callable(std::forward<callable_t>(_callable)),
          thread(_thread),
          state(state_go_to_core) {
        assert_good_thread_id(thread);
    }

    void run() {
        state = state_go_to_core;
        if (continue_on_thread(thread, this)) {
            do_perform_job();
        }
    }

    void do_perform_job() {
        rassert(thread == get_thread_id());
        callable();
        do_return_home();
    }

    // We go back to the home thread because it's nice to libtcmalloc
    // if we free memory on the same thread upon which it was
    // allocated.
    void do_return_home() {
        state = state_go_home;

        DEBUG_VAR bool no_switch = continue_on_thread(home_thread(), this);
        rassert(!no_switch);
    }

    void on_thread_switch() {
        switch (state) {
            case state_go_to_core:
                do_perform_job();
                return;
            case state_go_home:
                delete this;
                return;
            default:
                unreachable("Bad state.");
        }
    }
};

/* API to allow a nicer way of performing jobs on other cores than subclassing
from thread_message_t. Call do_on_thread() with an object and a method for that object.
The method will be called on the other thread. */

/**
 * @brief Execute a callable on a specific thread.
 *
 * Executes the provided callable on the specified thread. If called from the
 * target thread, executes immediately. Otherwise, creates a thread message
 * that executes on the target thread and returns to the home thread.
 *
 * @tparam callable_t Type of the callable to execute (function, lambda, functor).
 * @param thread The thread ID on which to execute the callable.
 * @param callable The callable to execute (moved into the message).
 *
 * Example:
 * @code
 * threadnum_t target = 2;
 * do_on_thread(target, [](){ std::cout << "Running on thread 2\n"; });
 * @endcode
 *
 * Example with capturing lambda:
 * @code
 * int value = 42;
 * do_on_thread(target, [value](){ process(value); });
 * @endcode
 */

template <class callable_t>
void do_on_thread(threadnum_t thread, callable_t &&callable) {
    assert_good_thread_id(thread);

    if (thread == get_thread_id()) {
        // Run the function directly since we are already in the requested thread
        callable();
    } else {
        thread_doer_t<callable_t> *fsm
            = new thread_doer_t<callable_t>(std::forward<callable_t>(callable),
                                            thread);
        fsm->run();
    }
}


#endif  // DO_ON_THREAD_HPP_
