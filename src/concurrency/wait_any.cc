#include "concurrency/wait_any.hpp"
#include "concurrency/signal.hpp"

class notifier_t {
public:
    notifier_t() : to_wake(NULL), notified_already(false) { }
    coro_t *to_wake;
    bool notified_already;
    void operator()() {
        if (!notified_already) {
            notified_already = true;
            if (to_wake) to_wake->notify_sometime();
        }
    }
    void wait() {
        if (!notified_already) {
            to_wake = coro_t::self();
            coro_t::wait();
        }
    }
};

void wait_any_lazily_unordered(signal_t *s1) {
    s1->wait_lazily_unordered();
}

void wait_any_lazily_unordered(signal_t *s1, signal_t *s2) {
    notifier_t n;
    if (s1->is_pulsed()) return;
    signal_t::subscription_t sub1(boost::ref(n), s1);
    if (s2->is_pulsed()) return;
    signal_t::subscription_t sub2(boost::ref(n), s2);
    n.wait();
}

void wait_any_lazily_unordered(signal_t *s1, signal_t *s2, signal_t *s3) {
    notifier_t n;
    if (s1->is_pulsed()) return;
    signal_t::subscription_t sub1(boost::ref(n), s1);
    if (s2->is_pulsed()) return;
    signal_t::subscription_t sub2(boost::ref(n), s2);
    if (s3->is_pulsed()) return;
    signal_t::subscription_t sub3(boost::ref(n), s3);
    n.wait();
}

void wait_any_lazily_unordered(signal_t *s1, signal_t *s2, signal_t *s3, signal_t *s4) {
    notifier_t n;
    if (s1->is_pulsed()) return;
    signal_t::subscription_t sub1(boost::ref(n), s1);
    if (s2->is_pulsed()) return;
    signal_t::subscription_t sub2(boost::ref(n), s2);
    if (s3->is_pulsed()) return;
    signal_t::subscription_t sub3(boost::ref(n), s3);
    if (s4->is_pulsed()) return;
    signal_t::subscription_t sub4(boost::ref(n), s4);
    n.wait();
}

void wait_any_lazily_unordered(signal_t *s1, signal_t *s2, signal_t *s3, signal_t *s4, signal_t *s5) {
    notifier_t n;
    if (s1->is_pulsed()) return;
    signal_t::subscription_t sub1(boost::ref(n), s1);
    if (s2->is_pulsed()) return;
    signal_t::subscription_t sub2(boost::ref(n), s2);
    if (s3->is_pulsed()) return;
    signal_t::subscription_t sub3(boost::ref(n), s3);
    if (s4->is_pulsed()) return;
    signal_t::subscription_t sub4(boost::ref(n), s4);
    if (s5->is_pulsed()) return;
    signal_t::subscription_t sub5(boost::ref(n), s5);
    n.wait();
}
