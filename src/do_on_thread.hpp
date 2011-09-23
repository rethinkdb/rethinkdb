#ifndef __DO_ON_THREAD_HPP__
#define __DO_ON_THREAD_HPP__

#include "arch/runtime/runtime.hpp"
#include "utils.hpp"

/* Functions to do something on another core in a way that is more convenient than
continue_on_thread() is. */

template<class callable_t>
struct thread_doer_t :
    public thread_message_t,
    public home_thread_mixin_t
{
    callable_t callable;
    int thread;
    enum state_t {
        state_go_to_core,
        state_go_home
    } state;
    
    thread_doer_t(const callable_t& _callable, int _thread)
        : callable(_callable), thread(_thread), state(state_go_to_core) {
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
    
    void do_return_home() {
        state = state_go_home;
        if (continue_on_thread(home_thread(), this)) delete this;
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
The method will be called on the other thread. If the thread to call the method on is
the current thread, returns the method's return value. Otherwise, returns false. */

template<class callable_t>
void do_on_thread(int thread, const callable_t &callable) {
    assert_good_thread_id(thread);
    // TODO: if we are currently on `thread`, we shouldn't need to allocate memory to do this.
    thread_doer_t<callable_t> *fsm = new thread_doer_t<callable_t>(callable, thread);
    fsm->run();
}


template <class callable_t>
class later_doer_t : public thread_message_t {
public:
    later_doer_t(const callable_t& callable, int thread) : callable_(callable), thread_(thread) { }

    void run() {
        if (continue_on_thread(thread_, this)) {
            call_later_on_this_thread(this);
        }
    }

private:
    void on_thread_switch() {
        rassert(thread_ == get_thread_id());
        callable_();
        delete this;
    }

    callable_t callable_;
    int thread_;

    DISABLE_COPYING(later_doer_t);
};

// Like do_on_thread, but if it's the current thread, does it later instead of now.
template <class callable_t>
void do_later_on_thread(int thread, const callable_t& callable) {
    (new later_doer_t<callable_t>(callable, thread))->run();
}



#endif  // __DO_ON_THREAD_HPP__
