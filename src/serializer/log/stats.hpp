#ifndef SERIALIZER_LOG_STATS_HPP_
#define SERIALIZER_LOG_STATS_HPP_

#include "perfmon/perfmon.hpp"

struct log_serializer_stats_t {
    perfmon_collection_t serializer_collection;
    explicit log_serializer_stats_t(perfmon_collection_t *perfmon_collection);

    perfmon_duration_sampler_t pm_serializer_block_reads;
    perfmon_counter_t pm_serializer_index_reads;
    perfmon_counter_t pm_serializer_block_writes;
    perfmon_duration_sampler_t pm_serializer_index_writes;
    perfmon_sampler_t pm_serializer_index_writes_size;

    /* used in serializer/log/extent_manager.cc */
    perfmon_counter_t pm_extents_in_use;
    perfmon_counter_t pm_bytes_in_use;

    /* used in serializer/log/lba/extent.cc */
    perfmon_counter_t pm_serializer_lba_extents;

    /* used in serializer/log/data_block_manager.cc */
    perfmon_counter_t pm_serializer_data_extents;
    perfmon_counter_t pm_serializer_data_extents_allocated;
    perfmon_counter_t pm_serializer_data_extents_reclaimed;
    perfmon_counter_t pm_serializer_data_extents_gced;
    perfmon_counter_t pm_serializer_data_blocks_written;
    perfmon_counter_t pm_serializer_old_garbage_blocks;
    perfmon_counter_t pm_serializer_old_total_blocks;

    /* used in serializer/log/lba/lba_list.cc */
    perfmon_counter_t pm_serializer_lba_gcs;

    perfmon_membership_t parent_collection_membership;
    perfmon_multi_membership_t stats_membership;
};

#endif /* SERIALIZER_LOG_STATS_HPP_ */
