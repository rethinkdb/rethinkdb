#ifndef CONCURRENCY_WAIT_ANY_HPP_
#define CONCURRENCY_WAIT_ANY_HPP_

#include <vector>

#include "concurrency/signal.hpp"
#include "containers/intrusive_list.hpp"


/* Monitors multiple signals; becomes pulsed if any individual signal becomes
pulsed. */

class wait_any_subscription_t;

class wait_any_t : public signal_t {
public:
    wait_any_t();
    explicit wait_any_t(signal_t *s1);
    wait_any_t(signal_t *s1, signal_t *s2);
    wait_any_t(signal_t *s1, signal_t *s2, signal_t *s3);
    wait_any_t(signal_t *s1, signal_t *s2, signal_t *s3, signal_t *s4);
    wait_any_t(signal_t *s1, signal_t *s2, signal_t *s3, signal_t *s4, signal_t *s5);

    ~wait_any_t();

    void add(signal_t *s);
private:
    class wait_any_subscription_t : public signal_t::subscription_t, public intrusive_list_node_t<wait_any_subscription_t> {
    public:
        wait_any_subscription_t() : parent_(NULL) { }

        void init(wait_any_t *parent);
        virtual void run();
    private:
        wait_any_t *parent_;
        DISABLE_COPYING(wait_any_subscription_t);
    };

    static const size_t default_preallocated_subs = 3;

    void pulse_if_not_already_pulsed();

    intrusive_list_t<wait_any_subscription_t> subs;

    wait_any_subscription_t sub_storage[default_preallocated_subs];

    DISABLE_COPYING(wait_any_t);
};

/* Waits for the first signal to become pulsed. If the second signal becomes
pulsed, stops waiting and throws `interrupted_exc_t`. */
void wait_interruptible(signal_t *signal, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

#endif /* CONCURRENCY_WAIT_ANY_HPP_ */
