// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_CPU_SHARDING_HPP_
#define CLUSTERING_TABLE_CONTRACT_CPU_SHARDING_HPP_

#include "protocol_api.hpp"
#include "region/region.hpp"

#define CPU_SHARDING_FACTOR 8

region_t cpu_sharding_subspace(int subregion_number);
int get_cpu_shard_number(const region_t &region);

class multistore_ptr_t {
public:
    virtual ~multistore_ptr_t() { }
    store_view_t *shards[CPU_SHARDING_FACTOR];
};

#endif /* CLUSTERING_TABLE_CONTRACT_CPU_SHARDING_HPP_ */

