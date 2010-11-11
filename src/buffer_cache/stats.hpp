#ifndef __BUFFER_CACHE_STATS_HPP__
#define __BUFFER_CACHE_STATS_HPP__

#include "perfmon.hpp"

extern perfmon_counter_t
    pm_n_bufs_acquired,
    pm_n_bufs_ready,
    pm_n_bufs_released;

extern perfmon_counter_t
    pm_n_blocks_in_memory,
    pm_n_blocks_dirty,
    pm_n_blocks_total;

extern perfmon_counter_t
    pm_n_cows_made,
    pm_n_cows_destroyed;

#endif /* __BUFFER_CACHE_STATS_HPP__ */
