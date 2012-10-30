#ifndef ARCH_RUNTIME_RUNTIME_UTILS_HPP_
#define ARCH_RUNTIME_RUNTIME_UTILS_HPP_

#include <stdint.h>

#include <string>

#include "containers/intrusive_list.hpp"

typedef int fd_t;
#define INVALID_FD fd_t(-1)

class linux_thread_message_t : public intrusive_list_node_t<linux_thread_message_t> {
public:
    linux_thread_message_t() 
#ifndef NDEBUG
        : reloop_count_(0) 
#endif
        { }
    virtual void on_thread_switch() = 0;
protected:
    virtual ~linux_thread_message_t() {}
private:
    friend class linux_message_hub_t;
#ifndef NDEBUG
    int reloop_count_;
#endif
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

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#endif
        // Allocate the action inside this object, if possible, or the heap otherwise
        if (sizeof(callable_action_instance_t<Callable>) > CALLABLE_CUTOFF_SIZE) {
            action_ = new callable_action_instance_t<Callable>(action);
            action_on_heap = true;
        } else {
            action_ = new (action_data) callable_action_instance_t<Callable>(action);
            action_on_heap = false;
        }
#ifdef __clang__
#pragma clang diagnostic pop
#endif
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

// More pollution of runtime_utils.hpp.
#ifndef NDEBUG

/* If `ASSERT_NO_CORO_WAITING;` appears at the top of a block, then it is illegal
to call `coro_t::wait()`, `coro_t::spawn_now_dangerously()`, or `coro_t::notify_now()`
within that block and any attempt to do so will be a fatal error. */
#define ASSERT_NO_CORO_WAITING assert_no_coro_waiting_t assert_no_coro_waiting_var(__FILE__, __LINE__)

/* If `ASSERT_FINITE_CORO_WAITING;` appears at the top of a block, then code
within that block may call `coro_t::spawn_now_dangerously()` or `coro_t::notify_now()` but
not `coro_t::wait()`. This is because `coro_t::spawn_now_dangerously()` and
`coro_t::notify_now()` will return control directly to the coroutine that called
then. */
#define ASSERT_FINITE_CORO_WAITING assert_finite_coro_waiting_t assert_finite_coro_waiting_var(__FILE__, __LINE__)

/* Implementation support for `ASSERT_NO_CORO_WAITING` and `ASSERT_FINITE_CORO_WAITING` */
struct assert_no_coro_waiting_t {
    assert_no_coro_waiting_t(const std::string&, int);
    ~assert_no_coro_waiting_t();
};
struct assert_finite_coro_waiting_t {
    assert_finite_coro_waiting_t(const std::string&, int);
    ~assert_finite_coro_waiting_t();
};

#else  // NDEBUG

/* In release mode, these assertions are no-ops. */
#define ASSERT_NO_CORO_WAITING do { } while (0)
#define ASSERT_FINITE_CORO_WAITING do { } while (0)

#endif  // NDEBUG


#endif // ARCH_RUNTIME_RUNTIME_UTILS_HPP_
