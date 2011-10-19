#ifndef __CONCURRENCY_COUNT_DOWN_LATCH_HPP__
#define __CONCURRENCY_COUNT_DOWN_LATCH_HPP__

#include "concurrency/signal.hpp"

/* A count_down_latch_t pulses its signal_t only after its count_down() method has
been called a certain number of times. It is safe to call the count_down() method
on any thread.

NOTE: This class might go away, because there's usually a better way to do
whatever it is you want to do. */

class count_down_latch_t : public signal_t {
public:
    explicit count_down_latch_t(size_t _count) : count(_count) { }

    void count_down();
private:
    void do_count_down();

    size_t count;
    DISABLE_COPYING(count_down_latch_t);
};

#endif // __CONCURRENCY_COUNT_DOWN_LATCH_HPP__
