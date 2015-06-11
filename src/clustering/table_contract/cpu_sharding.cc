// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/cpu_sharding.hpp"

static const uint64_t CPU_SHARD_WIDTH = HASH_REGION_HASH_SIZE / CPU_SHARDING_FACTOR;

region_t cpu_sharding_subspace(int subregion_number) {
    guarantee(subregion_number >= 0);
    guarantee(subregion_number < CPU_SHARDING_FACTOR);

    /* Changing this implementation would break backwards compatibility in the disk
    format. */

    // We have to be careful with the math here, to avoid overflow.
    uint64_t beg = CPU_SHARD_WIDTH * subregion_number;
    uint64_t end = subregion_number + 1 == CPU_SHARDING_FACTOR
        ? HASH_REGION_HASH_SIZE : beg + CPU_SHARD_WIDTH;

    return region_t(beg, end, key_range_t::universe());
}

int get_cpu_shard_number(const region_t &region) {
    int subregion_number = region.beg / CPU_SHARD_WIDTH;
    guarantee(region.beg == subregion_number * CPU_SHARD_WIDTH);
    guarantee(region.end == (
        subregion_number + 1 == CPU_SHARDING_FACTOR
            ? HASH_REGION_HASH_SIZE
            : region.beg + CPU_SHARD_WIDTH));
    return subregion_number;
}

int get_cpu_shard_approx_number(const region_t &region) {
    return region.beg / CPU_SHARD_WIDTH;
}

