// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "buffer_cache/mirrored/flush_time_randomizer.hpp"

#include <stdlib.h>

#include <algorithm>

#include "utils.hpp"

flush_time_randomizer_t::flush_time_randomizer_t(int _flush_timer_ms)
    : rng(),
      flush_timer_ms(_flush_timer_ms),
      first_time_interval(flush_timer_ms == 1 ? 1 : 1 + rng.randint(flush_timer_ms - 1)),
      done_first_time_interval(false) {
    guarantee(flush_timer_ms > 0 || flush_timer_ms == NEVER_FLUSH);
}

int flush_time_randomizer_t::next_time_interval() {
    guarantee(flush_timer_ms != NEVER_FLUSH);

    if (!done_first_time_interval) {
        done_first_time_interval = true;
        return first_time_interval;
    }

    // We have about a 19/20 chance of returning flush_timer_ms.
    if (flush_timer_ms > 1 && rng.randint(20) == 0) {
        // Otherwise, we return a value uniformly in (flush_timer_ms / 2, flush_timer_ms].
        return flush_timer_ms - rng.randint(flush_timer_ms / 2);
    }
    return flush_timer_ms;
}
