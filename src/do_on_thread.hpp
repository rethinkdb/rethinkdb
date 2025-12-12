// Copyright 2010-2013 RethinkDB, all rights reserved.

/**
 * @file do_on_thread.hpp
 * @brief Convenient API for executing functions on specific threads/cores.
 *
 * This header provides a simpler alternative to continue_on_thread() for
 * executing arbitrary callables on other cores. Instead of subclassing
 * thread_message_t, you can simply pass a callable object or lambda.
 */

#ifndef DO_ON_THREAD_HPP_
#define DO_ON_THREAD_HPP_

#include "arch/runtime/runtime.hpp"
#include "utils.hpp"

/**
 * @defgroup ThreadExecution Thread Execution
 * @brief Utilities for executing code on specific threads/cores.
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
 * @ingroup ThreadExecution
 * @brief Executes a callable on a specific thread/core.
 *
 * Provides a convenient way to execute arbitrary code on another thread without
 * having to subclass thread_message_t. If the callable is already on the target
 * thread, it executes immediately. Otherwise, it creates a thread_doer_t FSM
 * to manage the execution across threads.
 *
 * The execution happens in two phases:
 * 1. Switch to the target thread and execute the callable
 * 2. Return to the home thread to clean up (ensures memory is freed on the correct thread)
 *
 * @tparam callable_t The type of callable (function pointer, lambda, functor, etc.).
 * @param thread The target thread number to execute the callable on.
 * @param callable The callable to execute. Moved into the thread_doer_t.
 *
 * @code
 * // Execute lambda on thread 1
 * threadnum_t target = threadnum_t(1);
 * int result = 0;
 * do_on_thread(target, [&result]() {
 *     result = some_expensive_computation();
 * });
 * // After do_on_thread returns, result contains the computation result
 * @endcode
 *
 * @code
 * // Execute a method call on a different thread
 * class MyClass {
 * public:
 *     void process() { /* ... */ }
 * };
 *
 * MyClass obj;
 * do_on_thread(threadnum_t(2), [&obj]() { obj.process(); });
 * @endcode
 *
 * @code
 * // Execute a named function
 * void cleanup_thread_resources() { /* ... */ }
 * do_on_thread(threadnum_t(3), cleanup_thread_resources);
 * @endcode
 *
 * @note The callable must be exception-safe, as exceptions are not propagated
 *       back to the calling thread.
 * @note The callable is moved, not copied. Use std::move for large objects or
 *       ensure move semantics are efficient.
 * @note If thread equals the current thread ID, the callable is executed
 *       immediately without any inter-thread communication.
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
