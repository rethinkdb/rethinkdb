#ifndef __CLUSTERING_REACTOR_DELAYED_PUMP_HPP__
#define __CLUSTERING_REACTOR_DELAYED_PUMP_HPP__

#include "concurrency/mutex_assertion.hpp"
#include "concurrency/auto_drainer.hpp"

/* When you call `delayed_pump_t::run_eventually()`, it makes sure that the
function you passed to `delayed_pump_t::delayed_pump_t()` will get run at least
once at some point in the future. It guarantees that the function will never be
running twice concurrently, and it's safe to call `run_eventually()` from within
the function. It's useful for sync operations and other idempotent actions. */

class delayed_pump_t : public home_thread_mixin_t {
public:
    delayed_pump_t(const boost::function<void()> &f) :
        function(f), state(state_off) { }
    void run_eventually() {
        mutex_assertion_t::acq_t acq(&lock);
        switch (state) {
            case state_off: {
                coro_t::spawn_sometime(boost::bind(&delayed_pump_t::call_function,
                    this, auto_drainer_t::lock_t(&drainer)));
                state = state_scheduled;
                break;
            }
            case state_scheduled: {
                /* Ignore */
                break;
            }
            case state_running: {
                /* Make `call_function()` go around the loop againm */
                state = state_scheduled;
                break;
            }
        }
    }
private:
    enum state_t {
        state_off,
        state_scheduled,
        state_running
    };
    void call_function(auto_drainer_t::lock_t) {
        mutex_assertion_t::acq_t acq(&lock);
        rassert(state == state_scheduled);
        while (state == state_scheduled) {
            state = state_running;
            mutex_assertion_t::acq_t::temporary_release_t release(&acq);
            function();
        }
    }
    boost::function<void()> function;
    state_t state;
    mutex_assertion_t lock;
    auto_drainer_t drainer;
};

#endif /* __CLUSTERING_REACTOR_DELAYED_PUMP_HPP__ */