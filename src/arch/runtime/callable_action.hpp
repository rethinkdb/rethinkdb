#ifndef ARCH_RUNTIME_CALLABLE_ACTION_HPP_
#define ARCH_RUNTIME_CALLABLE_ACTION_HPP_

#include <utility>

#include "arch/compiler.hpp"
#include "errors.hpp"

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
    explicit callable_action_instance_t(Callable &&callable)
        : callable_(std::forward<Callable>(callable)) { }

    void run_action() { callable_(); }

private:
    Callable callable_;
};

class callable_action_wrapper_t {
public:
    callable_action_wrapper_t();
    ~callable_action_wrapper_t();

    template<class Callable>
    void reset(Callable &&action)
    {
        rassert(action_ == NULL);

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#endif
        // Allocate the action inside this object, if possible, or the heap otherwise
        if (sizeof(callable_action_instance_t<Callable>) > CALLABLE_CUTOFF_SIZE) {
            action_ = new callable_action_instance_t<Callable>(
                    std::forward<Callable>(action));
            action_on_heap = true;
        } else {
            action_ = new (action_data) callable_action_instance_t<Callable>(
                    std::forward<Callable>(action));
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
    ATTR_ALIGNED(sizeof(void*)) char action_data[CALLABLE_CUTOFF_SIZE];

    DISABLE_COPYING(callable_action_wrapper_t);
};


#endif  // ARCH_RUNTIME_CALLABLE_ACTION_HPP_
