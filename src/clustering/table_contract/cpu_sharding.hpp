// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_CPU_SHARDING_HPP_
#define CLUSTERING_TABLE_CONTRACT_CPU_SHARDING_HPP_

#include "clustering/immediate_consistency/history.hpp"
#include "protocol_api.hpp"
#include "region/region.hpp"

class store_t;

/* Changing this number would break backwards compatibility in the disk format. */
#define CPU_SHARDING_FACTOR 8

/* `cpu_sharding_subspace()` returns a `region_t` that contains the full key-range space
but only 1/CPU_SHARDING_FACTOR of the shard space. */
region_t cpu_sharding_subspace(int subregion_number);

/* `get_cpu_shard_number()` is the reverse of `cpu_sharding_subspace()`; it returns the
subregion number for `region`'s hash subspace. It ignores `region`'s key boundaries. If
`region`'s hash subspace doesn't exactly correspond to a specific CPU sharding region, it
crashes. */
int get_cpu_shard_number(const region_t &region);

/* `multistore_ptr_t` is a bundle of `store_view_t`s, one for each CPU shard. The rule
is that `get_cpu_sharded_store(i)->get_region() == cpu_sharding_subspace(i)`. The
individual stores' home threads may be different from the `multistore_ptr_t`'s home
thread. */
class multistore_ptr_t : public home_thread_mixin_t {
public:
    virtual ~multistore_ptr_t() { }

    virtual branch_history_manager_t *get_branch_history_manager() = 0;

    virtual store_view_t *get_cpu_sharded_store(size_t i) = 0;

    /* The `sindex_manager_t` uses this interface to get at the underlying `store_t`s so
    it can create and destroy sindexes on them. The `table_contract` code should never
    use it, and some unit tests will return `nullptr` from here. */
    virtual store_t *get_underlying_store(size_t i) = 0;
};

#endif /* CLUSTERING_TABLE_CONTRACT_CPU_SHARDING_HPP_ */

