// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_raft/cpu_sharding.hpp"

region_t cpu_sharding_subspace(int subregion_number) {
    guarantee(subregion_number >= 0);
    guarantee(subregion_number < CPU_SHARDING_FACTOR);

    // We have to be careful with the math here, to avoid overflow.
    uint64_t width = HASH_REGION_HASH_SIZE / CPU_SHARDING_FACTOR;

    uint64_t beg = width * subregion_number;
    uint64_t end = subregion_number + 1 == CPU_SHARDING_FACTOR
        ? HASH_REGION_HASH_SIZE : beg + width;

    return region_t(beg, end, key_range_t::universe());
}

int get_cpu_shard_number(const region_t &region) {
    uint64_t width = HASH_REGION_HASH_SIZE / CPU_SHARDING_FACTOR;
    guarantee(region.beg % width == 0);
    int subregion_number = region.beg / width;
    guarantee(region.end == (
        subregion_number + 1 == CPU_SHARDING_FACTOR
            ? HASH_REGION_HASH_SIZE
            : region.beg + width));
    return subregion_number;
}

