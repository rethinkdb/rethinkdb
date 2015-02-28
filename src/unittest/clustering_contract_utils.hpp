// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef UNITTEST_CLUSTERING_CONTRACT_UTILS_HPP_
#define UNITTEST_CLUSTERING_CONTRACT_UTILS_HPP_

#include "clustering/table_contract/contract_metadata.hpp"
#include "clustering/table_contract/cpu_sharding.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

/* The `contract_coordinator_t` and `contract_executor_t` assume no contracts span
multiple CPU shards. To reduce verbosity, the functions in this file automatically set up
many parallel contracts/branches/etc., one for each CPU shard. `cpu_branch_ids_t`,
`cpu_contract_ids_t`, and `cpu_contracts_t` are sets of branch IDs, contract IDs, or
contracts, one for each CPU shard. */
class cpu_branch_ids_t {
public:
    key_range_t range;
    branch_id_t branch_ids[CPU_SHARDING_FACTOR];
};
class cpu_contract_ids_t {
public:
    contract_id_t contract_ids[CPU_SHARDING_FACTOR];
};
class cpu_contracts_t {
public:
    contract_t contracts[CPU_SHARDING_FACTOR];
};

/* `quick_version_map()` is a minimum-verbosity way to construct a
`region_map_t<version_t>`. Use it something like this:

    region_map_t<version_t> vers_map_for_cpu_shard_N = quick_version_map(N, {
        {"ABC", &my_first_branch, 25},
        {"DE", &my_second_branch, 28}
        });

This creates a `region_map_t` on the branch `my_first_branch` at timestamp 25 in the
first key-range, and on the branch `my_second_branch` at timestamp 28 in the second
key-range. Note that it only operates on one CPU shard at a time. */
struct quick_version_map_args_t {
public:
    const char *quick_range_spec;
    const cpu_branch_ids_t *branch;
    int timestamp;
};
region_map_t<version_t> quick_version_map(
        size_t which_cpu_subspace,
        std::initializer_list<quick_version_map_args_t> qvms);

/* `quick_branch()` is a convenience function to construct a collection of CPU-sharded
branches. Usage looks something like this:

    cpu_branch_ids_t merged_branch = quick_branch(&branch_history, {
        {"ABC", &left_branch, 25},
        {"DE", &rigth_branch, 28}
        });

This creates a new branch by merging `left_branch` at timestamp 25 with `right_branch` at
timestamp 28. */
cpu_branch_ids_t quick_branch(
        branch_history_t *bhist,
        std::initializer_list<quick_version_map_args_t> origin);

/* `quick_contract_*()` are convenience functions to create collections of CPU-sharded
contracts. */
cpu_contracts_t quick_contract_simple(
        const std::set<server_id_t> &voters,
        const server_id_t &primary,
        const cpu_branch_ids_t *branch);
cpu_contracts_t quick_contract_extra_replicas(
        const std::set<server_id_t> &voters,
        const std::set<server_id_t> &extras,
        const server_id_t &primary,
        const cpu_branch_ids_t *branch);
cpu_contracts_t quick_contract_no_primary(
        const std::set<server_id_t> &voters,
        const cpu_branch_ids_t *branch);
cpu_contracts_t quick_contract_hand_over(
        const std::set<server_id_t> &voters,
        const server_id_t &primary,
        const server_id_t &hand_over,
        const cpu_branch_ids_t *branch);
cpu_contracts_t quick_contract_temp_voters(
        const std::set<server_id_t> &voters,
        const std::set<server_id_t> &temp_voters,
        const server_id_t &primary,
        const cpu_branch_ids_t *branch);
cpu_contracts_t quick_contract_temp_voters_hand_over(
        const std::set<server_id_t> &voters,
        const std::set<server_id_t> &temp_voters,
        const server_id_t &primary,
        const server_id_t &hand_over,
        const cpu_branch_ids_t *branch);

} /* namespace unittest */

#endif /* UNITTEST_CLUSTERING_CONTRACT_UTILS_HPP_ */

