#include "concurrency/count_down_latch.hpp"

#include <boost/bind.hpp>

#include "do_on_thread.hpp"

void count_down_latch_t::count_down() {
    do_on_thread(home_thread(), boost::bind(&count_down_latch_t::do_count_down, this));
}

void count_down_latch_t::do_count_down() {
    rassert(count > 0);
    --count;
    if (count == 0) {
        pulse();
    }
}
