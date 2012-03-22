#ifndef __CONCURRENCY_WAIT_ANY_HPP__
#define __CONCURRENCY_WAIT_ANY_HPP__

#include "concurrency/signal.hpp"
#include <boost/ptr_container/ptr_vector.hpp>

/* Monitors multiple signals; becomes pulsed if any individual signal becomes
pulsed. */

class wait_any_t : public signal_t {
public:
    wait_any_t();
    explicit wait_any_t(signal_t *s1);
    wait_any_t(signal_t *s1, signal_t *s2);
    wait_any_t(signal_t *s1, signal_t *s2, signal_t *s3);
    wait_any_t(signal_t *s1, signal_t *s2, signal_t *s3, signal_t *s4);
    wait_any_t(signal_t *s1, signal_t *s2, signal_t *s3, signal_t *s4, signal_t *s5);
    void add(signal_t *s);
private:
    boost::ptr_vector<signal_t::subscription_t> subs;
    void pulse_if_not_already_pulsed();
};

/* Waits for the first signal to become pulsed. If the second signal becomes
pulsed, stops waiting and throws `interrupted_exc_t`. */
void wait_interruptible(signal_t *signal, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

#endif /* __CONCURRENCY_WAIT_ANY_HPP__ */
