#ifndef __CONCURRENCY_RESETTABLE_COND_VAR_HPP__
#define __CONCURRENCY_RESETTABLE_COND_VAR_HPP__

#include "concurrency/cond_var.hpp"

/* resettable_cond_t is like cond_t except that it can be "un-pulsed" using the reset()
method. Because a signal_t can't actually be reset, you have to call get_signal()
explicitly to get its signal_t*. After you call reset(), a pointer to a fresh signal_t will
be returned by get_signal(). */

struct resettable_cond_t {
    resettable_cond_t() : state(false), signal(new cond_t) { }
    void pulse() {
        rassert(state == false);
        state = true;
        signal->pulse();
    }
    void reset() {
        rassert(state == true);
        state = false;
        signal.reset(new cond_t);
    }
    signal_t *get_signal() {
        return signal.get();
    }
private:
    bool state;
    boost::scoped_ptr<cond_t> signal;
};

#endif /* __CONCURRENCY_RESETTABLE_COND_VAR_HPP__ */
