#ifndef BUFFER_CACHE_STATS_HPP_
#define BUFFER_CACHE_STATS_HPP_

#include "perfmon_types.hpp"

extern perfmon_counter_t
    pm_n_blocks_in_memory,
    pm_n_blocks_dirty,
    pm_n_blocks_total;

#endif /* BUFFER_CACHE_STATS_HPP_ */
