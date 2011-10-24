#ifndef __CONCURRENCY_RESETTABLE_COND_VAR_HPP__
#define __CONCURRENCY_RESETTABLE_COND_VAR_HPP__

#include <boost/scoped_ptr.hpp>

class cond_t;
class signal_t;

/* resettable_cond_t is like cond_t except that it can be "un-pulsed" using the reset()
method. Because a signal_t can't actually be reset, you have to call get_signal()
explicitly to get its signal_t*. After you call reset(), a pointer to a fresh signal_t will
be returned by get_signal(). */

// It is also an anti-pattern.

class resettable_cond_t {
public:
    resettable_cond_t();
    ~resettable_cond_t();
    void pulse();
    void reset();
    signal_t *get_signal();
    void rethread(int new_thread);
private:
    bool state;
    boost::scoped_ptr<cond_t> signal;
};

#endif /* __CONCURRENCY_RESETTABLE_COND_VAR_HPP__ */
