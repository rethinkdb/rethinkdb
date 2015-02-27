// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/table_contract/coordinator.hpp"
#include "clustering/table_contract/cpu_sharding.hpp"
#include "unittest/unittest_utils.hpp"

/* This file is for unit testing the `contract_coordinator_t`. This is tricky to unit
test because the inputs and outputs are complicated, and we want to test many different
scenarios. So we have a bunch of helper functions and types for constructing test
scenarios.

The general outline of a test is as follows: Construct a `quick_coordinator_tester_t`.
Use its `set_config()`, `add_contract()`, and `add_ack()` methods to set up the scenario.
Call `coordinate()` and then use `check_contract()` to make sure the newly-created
contracts make sense. If desired, adjust the inputs and repeat. */

/* These functions are defined internally in `clustering/table_contract/coordinator.cc`,
and not declared in the header, so we have to declare them here. */
void calculate_all_contracts(
        const table_raft_state_t &old_state,
        watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *acks,
        std::set<contract_id_t> *remove_contracts_out,
        std::map<contract_id_t, std::pair<region_t, contract_t> > *add_contracts_out);
void calculate_branch_history(
        const table_raft_state_t &old_state,
        watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *acks,
        const std::set<contract_id_t> &remove_contracts,
        const std::map<contract_id_t, std::pair<region_t, contract_t> > &add_contracts,
        std::set<branch_id_t> *remove_branches_out,
        branch_history_t *add_branches_out);

namespace unittest {

/* The `contract_coordinator_t` assumes none of its contracts span multiple CPU shards.
To reduce verbosity, the `quick_coordinator_tester_t` automatically sets up many parallel
contracts/branches/acks/etc., one for each CPU shard. `cpu_branch_ids_t` and
`cpu_contract_ids_t` are sets of branch or contract IDs, one for each CPU shard. */
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
        std::initializer_list<quick_version_map_args_t> qvms) {
    std::vector<std::pair<region_t, version_t> > parts;
    for (const quick_version_map_args_t &qvm : qvms) {
        key_range_t range = quick_range(qvm.quick_range_spec);
        region_t region = region_intersection(
            region_t(range), cpu_sharding_subspace(which_cpu_subspace));
        version_t version;
        if (qvm.branch == nullptr) {
            guarantee(qvm.timestamp == 0);
            version = version_t::zero();
        } else {
            version = version_t(
                qvm.branch->branch_ids[which_cpu_subspace],
                state_timestamp_t::from_num(qvm.timestamp));
        }
        parts.push_back({ region, version });
    }
    return region_map_t<version_t>(parts.begin(), parts.end());
}

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
        std::initializer_list<quick_version_map_args_t> origin) {
    cpu_branch_ids_t res;

    /* Compute the region of the new "branch" */
    res.range.left = quick_range(origin.begin()->quick_range_spec).left;
    res.range.right = key_range_t::right_bound_t(res.range.left);
    for (const quick_version_map_args_t &qvm : origin) {
        key_range_t range = quick_range(qvm.quick_range_spec);
        guarantee(key_range_t::right_bound_t(range.left) == res.range.right);
        res.range.right = range.right;
        if (qvm.branch != nullptr) {
            guarantee(region_is_superset(qvm.branch->range, range));
        }
    }

    /* Create birth certificates for the individual cpu-specific branches of the new
    "branch" */
    branch_birth_certificate_t bcs[CPU_SHARDING_FACTOR];
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        bcs[i].region = region_intersection(
            region_t(res.range), cpu_sharding_subspace(i));
        bcs[i].initial_timestamp = state_timestamp_t::zero();
        bcs[i].origin = quick_version_map(i, origin);
        
    }

    /* Register the branches */
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        branch_id_t bid = generate_uuid();
        bhist->branches.insert(std::make_pair(bid, bcs[i]));
        res.branch_ids[i] = bid;
    }

    return res;
}

/* `quick_contracts_*()` are convenience functions to create collections of CPU-sharded
contracts. */
cpu_contracts_t quick_contracts_simple(
        const std::set<server_id_t> &voters,
        const server_id_t &primary,
        const cpu_branch_ids_t *branch) {
    cpu_contracts_t res;
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        res.contracts[i].replicas = res.contracts[i].voters = voters;
        res.contracts[i].primary =
            boost::make_optional(contract_t::primary_t { primary, boost::none } );
        res.contracts[i].branch = branch->branch_ids[i];
    }
    return res;
}
cpu_contracts_t quick_contracts_extra_replicas(
        const std::set<server_id_t> &voters,
        const std::set<server_id_t> &extras,
        const server_id_t &primary,
        const cpu_branch_ids_t *branch) {
    cpu_contracts_t res;
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        res.contracts[i].replicas = res.contracts[i].voters = voters;
        res.contracts[i].replicas.insert(extras.begin(), extras.end());
        res.contracts[i].primary =
            boost::make_optional(contract_t::primary_t { primary, boost::none } );
        res.contracts[i].branch = branch->branch_ids[i];
    }
    return res;
}
        
cpu_contracts_t quick_contracts_no_primary(
        const std::set<server_id_t> &voters,   /* first voter is primary */
        const cpu_branch_ids_t *branch) {
    cpu_contracts_t res;
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        res.contracts[i].replicas = res.contracts[i].voters = voters;
        res.contracts[i].primary = boost::none;
        res.contracts[i].branch = branch->branch_ids[i];
    }
    return res;
}
cpu_contracts_t quick_contracts_handover(
        const std::set<server_id_t> &voters,
        const server_id_t &primary,
        const server_id_t &handover,
        const cpu_branch_ids_t *branch) {
    cpu_contracts_t res;
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        res.contracts[i].replicas = res.contracts[i].voters = voters;
        res.contracts[i].primary =
            boost::make_optional(contract_t::primary_t {
                primary, boost::make_optional(handover) } );
        res.contracts[i].branch = branch->branch_ids[i];
    }
    return res;
}
cpu_contracts_t quick_contracts_temp_voters(
        const std::set<server_id_t> &voters,
        const std::set<server_id_t> &temp_voters,
        const server_id_t &primary,
        const cpu_branch_ids_t *branch) {
    cpu_contracts_t res;
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        res.contracts[i].replicas = res.contracts[i].voters = voters;
        res.contracts[i].temp_voters = boost::make_optional(temp_voters);
        res.contracts[i].primary =
            boost::make_optional(contract_t::primary_t { primary, boost::none } );
        res.contracts[i].branch = branch->branch_ids[i];
    }
    return res;
}

struct quick_coordinator_tester_t {
    table_raft_state_t state;
    watchable_map_var_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> acks;

    /* `set_config()` is a fast way to change the Raft config. Use it something like
    this:

        tester.set_config({
            {"A", {s1, s2} },
            {"BC", {s2, s3} },
            {"DE", {s3, s1} }
            });

    This makes a config with three shards, each of which has a different primary and
    secondary. The first server in the list is always the primary. */
    struct quick_shard_args_t {
    public:
        const char *quick_range_spec;
        std::vector<server_id_t> replicas;
    };
    void set_config(std::initializer_list<quick_shard_args_t> qss) {
        table_config_and_shards_t cs;
        cs.config.database = generate_uuid();
        cs.config.name = name_string_t::guarantee_valid("test");
        cs.config.primary_key = "id";
        cs.config.write_ack_config.mode = write_ack_config_t::mode_t::majority;
        cs.config.durability = write_durability_t::HARD;

        key_range_t::right_bound_t prev_right(store_key_t::min());
        for (const quick_shard_args_t &qs : qss) {
            table_config_t::shard_t s;
            s.replicas.insert(qs.replicas.begin(), qs.replicas.end());
            s.primary_replica = qs.replicas.front();
            cs.config.shards.push_back(s);

            key_range_t range = quick_range(qs.quick_range_spec);
            guarantee(key_range_t::right_bound_t(range.left) == prev_right);
            if (!range.right.unbounded) {
                cs.shard_scheme.split_points.push_back(range.right.key);
            }
            prev_right = range.right;
        }
        guarantee(prev_right.unbounded);

        state.config = cs;
    }

    /* `add_contract()` adds the contracts in `contracts` to the state and returns the
    IDs generated for them.  */
    cpu_contract_ids_t add_contract(
            const char *quick_range_spec,
            const cpu_contracts_t &contracts) {
        cpu_contract_ids_t res;
        key_range_t range = quick_range(quick_range_spec);
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            res.contract_ids[i] = generate_uuid();
            state.contracts[res.contract_ids[i]] = std::make_pair(
                region_intersection(region_t(range), cpu_sharding_subspace(i)),
                contracts.contracts[i]);
        }
        return res;
    }

    /* `add_ack()` creates one ack for each contract in the CPU-sharded contract set.
    There are special variations for acks that need to attach a version or branch ID. */
    void add_ack(
            const server_id_t &server,
            const cpu_contract_ids_t &contracts,
            contract_ack_t::state_t st) {
        guarantee(st != contract_ack_t::state_t::secondary_need_primary &&
            st != contract_ack_t::state_t::primary_need_branch);
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            acks.set_key_no_equals(
                std::make_pair(server, contracts.contract_ids[i]),
                contract_ack_t(st));
        }
    }
    void add_ack(
            const server_id_t &server,
            const cpu_contract_ids_t &contracts,
            contract_ack_t::state_t st,
            const branch_history_t &branch_history,
            std::initializer_list<quick_version_map_args_t> version) {
        guarantee(st == contract_ack_t::state_t::secondary_need_primary);
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            contract_ack_t ack(st);
            ack.version = boost::make_optional(quick_version_map(i, version));
            ack.branch_history = branch_history;
            acks.set_key_no_equals(
                std::make_pair(server, contracts.contract_ids[i]),
                ack);
        }
    }
    void add_ack(
            const server_id_t &server,
            const cpu_contract_ids_t &contracts,
            contract_ack_t::state_t st,
            const branch_history_t &branch_history,
            cpu_branch_ids_t *branch) {
        guarantee(st == contract_ack_t::state_t::primary_need_branch);
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            contract_ack_t ack(st);
            ack.branch = boost::make_optional(branch->branch_ids[i]);
            ack.branch_history = branch_history;
            acks.set_key_no_equals(
                std::make_pair(server, contracts.contract_ids[i]),
                ack);
        }
    }

    /* Call `coordinate()` to run the contract coordinator logic on the inputs you've
    created. */
    void coordinate() {
        std::set<contract_id_t> remove_contracts;
        std::map<contract_id_t, std::pair<region_t, contract_t> > add_contracts;
        calculate_all_contracts(state, &acks, &remove_contracts, &add_contracts);
        std::set<branch_id_t> remove_branches;
        branch_history_t add_branches;
        calculate_branch_history(state, &acks, remove_contracts, add_contracts,
            &remove_branches, &add_branches);
        for (const contract_id_t &id : remove_contracts) {
            state.contracts.erase(id);
        }
        state.contracts.insert(add_contracts.begin(), add_contracts.end());
        for (const branch_id_t &id : remove_branches) {
            state.branch_history.branches.erase(id);
        }
        state.branch_history.branches.insert(
            add_branches.branches.begin(),
            add_branches.branches.end());
    }

    /* Use `check_contract()` to make sure that `coordinate()` produced reasonable
    contracts. Its interface mirrors that of `add_contract()`. */
    void check_contract(
            const char *quick_range_spec,
            const cpu_contracts_t &contracts) {
        key_range_t range = quick_range(quick_range_spec);
        bool found[CPU_SHARDING_FACTOR];
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            found[i] = false;
        }
        for (const auto &pair : state.contracts) {
            if (pair.second.first.inner == range) {
                size_t i = get_cpu_shard_number(pair.second.first);
                ASSERT_FALSE(found[i]);
                found[i] = true;
                const contract_t &expect = contracts.contracts[i];
                const contract_t &actual = pair.second.second;
                ASSERT_EQ(expect.replicas, actual.replicas);
                ASSERT_EQ(expect.voters, actual.voters);
                ASSERT_EQ(expect.temp_voters, actual.temp_voters);
                ASSERT_EQ(static_cast<bool>(expect.primary),
                    static_cast<bool>(actual.primary));
                if (static_cast<bool>(expect.primary)) {
                    ASSERT_EQ(expect.primary->server, actual.primary->server);
                    ASSERT_EQ(expect.primary->hand_over, actual.primary->hand_over);
                }
                ASSERT_EQ(expect.branch, actual.branch);
            }
        }
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            ASSERT_TRUE(found[i]);
        }
    }

    /* `check_same_contract()` checks that the same contract is still present, with the
    exact same ID. */
    void check_same_contract(const cpu_contract_ids_t &contract_ids) {
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            ASSERT_TRUE(state.contracts.count(contract_ids.contract_ids[i]) == 1);
        }
    }
};

TPTEST(ClusteringContractCoordinator, Static) {
}

} /* namespace unittest */

