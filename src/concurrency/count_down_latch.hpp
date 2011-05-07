#ifndef __CONCURRENCY_COUNT_DOWN_LATCH_HPP__
#define __CONCURRENCY_COUNT_DOWN_LATCH_HPP__

#include "concurrency/signal.hpp"

/* A count_down_latch_t pulses its signal_t only after its count_down() method has
been called a certain number of times. It is safe to call the cound_down() method
on any thread. */

struct count_down_latch_t : public signal_t {

    count_down_latch_t(size_t count) : count(count) { }

    void count_down() {
        do_on_thread(home_thread, boost::bind(&count_down_latch_t::do_count_down, this));
    }
private:
    void do_count_down() {
        rassert(count > 0);
        if (--count == 0) pulse();
    }

    size_t count;
    DISABLE_COPYING(count_down_latch_t);
};

#endif // __CONCURRENCY_COUNT_DOWN_LATCH_HPP__
