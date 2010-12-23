#ifndef __BUFFER_CACHE_STATS_HPP__
#define __BUFFER_CACHE_STATS_HPP__

#include "perfmon.hpp"

extern perfmon_counter_t
    pm_n_blocks_in_memory,
    pm_n_blocks_dirty,
    pm_n_blocks_total;

#endif /* __BUFFER_CACHE_STATS_HPP__ */
