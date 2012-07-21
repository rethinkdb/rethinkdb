#ifndef ARCH_RUNTIME_RUNTIME_UTILS_HPP_
#define ARCH_RUNTIME_RUNTIME_UTILS_HPP_

#include <stdint.h>

#include "containers/intrusive_list.hpp"

typedef int fd_t;
#define INVALID_FD fd_t(-1)

class linux_thread_message_t :
    public intrusive_list_node_t<linux_thread_message_t>
{
public:
    virtual void on_thread_switch() = 0;
protected:
    virtual ~linux_thread_message_t() {}
};

typedef linux_thread_message_t thread_message_t;

int get_cpu_count();

/* The below classes may be used to create a generic callable object without
  boost::function so as to avoid the heap allocation that boost::functions use.
  Allocate a callable_action_wrapper_t (preferrably on the stack), then assign
  any callable object into it.  The wrapper will only use the heap if it can't
  fit inside the internal pre-allocated buffer. */

#define CALLABLE_CUTOFF_SIZE 128

class callable_action_t {
public:
    virtual void run_action() = 0;
    callable_action_t() { }
    virtual ~callable_action_t() { }
private:
    DISABLE_COPYING(callable_action_t);
};

template<class Callable>
class callable_action_instance_t : public callable_action_t {
public:
    explicit callable_action_instance_t(const Callable& callable) : callable_(callable) { }

    void run_action() { callable_(); }

private:
    Callable callable_;
};

class callable_action_wrapper_t {
public:
    callable_action_wrapper_t();
    ~callable_action_wrapper_t();

    template<class Callable>
    void reset(const Callable& action)
    {
        rassert(action_ == NULL);

        // Allocate the action inside this object, if possible, or the heap otherwise
        if (sizeof(callable_action_instance_t<Callable>) > CALLABLE_CUTOFF_SIZE) {
            action_ = new callable_action_instance_t<Callable>(action);
            action_on_heap = true;
        } else {
            action_ = new (action_data) callable_action_instance_t<Callable>(action);
            action_on_heap = false;
        }
    }

    void reset();

    void run();

private:
    bool action_on_heap;
    callable_action_t *action_;
    char action_data[CALLABLE_CUTOFF_SIZE] __attribute__((aligned(sizeof(void*))));

    DISABLE_COPYING(callable_action_wrapper_t);
};

#ifndef NDEBUG
// Functions to keep track of running thread_message_t routines using get_clock_cycles
void enable_watchdog(); // Enables watchdog printouts (off by default to avoid command-line spam)
void start_watchdog(); // Starts time supervision
void disarm_watchdog(); // Suspends time supervision until the next pet or start
void pet_watchdog(); // Checks for long-running routines and restarts time supervision
#endif

#endif // ARCH_RUNTIME_RUNTIME_UTILS_HPP_
