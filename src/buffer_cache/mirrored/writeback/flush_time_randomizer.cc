#include "flush_time_randomizer.hpp"

#include <stdlib.h>
#include <algorithm>

#include "errors.hpp"
#include "utils2.hpp"

flush_time_randomizer_t::flush_time_randomizer_t(int flush_timer_ms_)
    : flush_timer_ms(flush_timer_ms_), first_time_interval(randint(std::max(1, flush_timer_ms))), done_first_time_interval(false) {
    rassert(flush_timer_ms >= 0 || flush_timer_ms == NEVER_FLUSH);
}

int flush_time_randomizer_t::next_time_interval() {
    rassert(flush_timer_ms != 0);

    if (!done_first_time_interval) {
        done_first_time_interval = true;
        return first_time_interval;
    }
    
    // We have about a 9/10 chance of returning flush_timer_ms.
    if (flush_timer_ms > 1 && randint(10) == 0) {
        // Otherwise, we return a value uniformly in (flush_timer_ms / 2, flush_timer_ms].
        return flush_timer_ms - randint(flush_timer_ms >> 1);
    }
    return flush_timer_ms;
}
