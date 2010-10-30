#ifndef __BUFFER_CACHE_STATS_HPP__
#define __BUFFER_CACHE_STATS_HPP__

#include "perfmon.hpp"

extern perfmon_counter_t
    pm_n_transactions_started,
    pm_n_transactions_ready,
    pm_n_transactions_committed,
    pm_n_transactions_completed;

extern perfmon_counter_t
    pm_n_bufs_acquired,
    pm_n_bufs_ready,
    pm_n_bufs_released;

extern perfmon_counter_t
    pm_n_blocks_in_memory,
    pm_n_blocks_dirty,
    pm_n_blocks_total;

#endif /* __BUFFER_CACHE_STATS_HPP__ */
