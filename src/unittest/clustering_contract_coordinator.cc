// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/table_contract/coordinator/calculate_contracts.hpp"
#include "clustering/table_contract/coordinator/calculate_misc.hpp"
#include "unittest/clustering_contract_utils.hpp"

/* This file is for unit testing the `contract_coordinator_t`. This is tricky to unit
test because the inputs and outputs are complicated, and we want to test many different
scenarios. So we have a bunch of helper functions and types for constructing test
scenarios.

The general outline of a test is as follows: Construct a `coordinator_tester_t`. Use its
`set_config()`, `add_contract()`, and `add_ack()` methods to set up the scenario. Call
`coordinate()` and then use `check_contract()` to make sure the newly-created contracts
make sense. If desired, adjust the inputs and repeat. */

namespace unittest {

class coordinator_tester_t {
public:
    explicit coordinator_tester_t(const std::set<server_id_t> &_all_servers) :
            all_servers(_all_servers) {
        for (const server_id_t &s1 : all_servers) {
            for (const server_id_t &s2 : all_servers) {
                connections.set_key(std::make_pair(s1, s2), empty_value_t());
            }
        }
    }

    /* `set_config()` is a fast way to change the Raft config. Use it something like
    this:

        tester.set_config({
            {"A", {s1, s2}, s1 },
            {"BC", {s2, s3}, s2 },
            {"DE", {s3, s1}, s3 }
            });

    This makes a config with three shards, each of which has a different primary and
    secondary. */
    struct quick_shard_args_t {
    public:
        const char *quick_range_spec;
        std::vector<server_id_t> replicas;
        server_id_t primary;
    };
    void set_config(std::initializer_list<quick_shard_args_t> qss) {
        table_config_and_shards_t cs;
        cs.config.basic.database = generate_uuid();
        cs.config.basic.name = name_string_t::guarantee_valid("test");
        cs.config.basic.primary_key = "id";
        cs.config.write_ack_config = write_ack_config_t::MAJORITY;
        cs.config.durability = write_durability_t::HARD;

        key_range_t::right_bound_t prev_right(store_key_t::min());
        for (const quick_shard_args_t &qs : qss) {
            table_config_t::shard_t s;
            s.all_replicas.insert(qs.replicas.begin(), qs.replicas.end());
            s.primary_replica = qs.primary;
            cs.config.shards.push_back(s);

            key_range_t range = quick_range(qs.quick_range_spec);
            guarantee(key_range_t::right_bound_t(range.left) == prev_right);
            if (!range.right.unbounded) {
                cs.shard_scheme.split_points.push_back(range.right.key());
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
        res.range = quick_range(quick_range_spec);
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            res.contract_ids[i] = generate_uuid();
            state.contracts[res.contract_ids[i]] = std::make_pair(
                region_intersection(region_t(res.range), cpu_sharding_subspace(i)),
                contracts.contracts[i]);
        }
        return res;
    }

    /* `set_current_branches()` sets the current branches. Normally this happens when
    the coordinator receives an ack with that branch in it from the primary for a given
    range during the initial branch registration of a new primary. */
    void set_current_branches(const cpu_branch_ids_t &branches) {
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            region_t reg = cpu_sharding_subspace(i);
            reg.inner = branches.range;
            state.current_branches.update(reg, branches.branch_ids[i]);
        }
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
            acks[contracts.contract_ids[i]][server] = contract_ack_t(st);
        }
    }
    void add_ack(
            const server_id_t &server,
            const cpu_contract_ids_t &contracts,
            contract_ack_t::state_t st,
            const branch_history_t &branch_history,
            std::initializer_list<quick_cpu_version_map_args_t> version) {
        guarantee(st == contract_ack_t::state_t::secondary_need_primary);
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            contract_ack_t ack(st);
            ack.version = boost::make_optional(quick_cpu_version_map(i, version));
            ack.branch_history = branch_history;
            acks[contracts.contract_ids[i]][server] = ack;
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
            ack[contracts.contract_ids[i]][server] = ack;
        }
    }

    /* `remove_ack()` removes the given server's acknowledgement of the given contract.
    This can be used to simulate e.g. server failures. */
    void remove_ack(const server_id_t &server, const cpu_contract_ids_t &contracts) {
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            acks[contracts.contract_ids[i]].erase(server);
            if (acks[contracts.contract_ids[i]].empty()) {
                acks.erase(contracts.contract_ids[i]);
            }
        }
    }

    /* `set_visibility()` adds or removes entries in the `connections` map. The first form
    sets bidirectional visibility between one server and all other servers; the second
    form sets unidirectional visibility from one specific server to one specific other
    server.  */
    void set_visibility(const server_id_t &s, bool visible) {
        for (const server_id_t &s2 : all_servers) {
            if (visible) {
                connections.set_key(std::make_pair(s, s2), empty_value_t());
                connections.set_key(std::make_pair(s2, s), empty_value_t());
            } else {
                connections.delete_key(std::make_pair(s, s2));
                connections.delete_key(std::make_pair(s2, s));
            }
        }
    }
    void set_visibility(const server_id_t &s1, const server_id_t &s2, bool visible) {
        if (visible) {
            connections.set_key(std::make_pair(s1, s2), empty_value_t());
        } else {
            connections.delete_key(std::make_pair(s1, s2));
        }
    }

    /* Call `coordinate()` to run the contract coordinator logic on the inputs you've
    created. */
    void coordinate() {
        std::set<contract_id_t> remove_contracts;
        std::map<contract_id_t, std::pair<region_t, contract_t> > add_contracts;
        std::map<region_t, branch_id_t> register_current_branches;
        calculate_all_contracts(state, acks, &connections, "",
            &remove_contracts, &add_contracts, &register_current_branches);
        std::set<branch_id_t> remove_branches;
        branch_history_t add_branches;
        calculate_branch_history(state, &acks, remove_contracts, add_contracts,
            register_current_branches, &remove_branches, &add_branches);
        for (const contract_id_t &id : remove_contracts) {
            state.contracts.erase(id);
            /* Clean out acks for obsolete contract */
            std::set<server_id_t> servers;
            acks.read_all(
            [&](const std::pair<server_id_t, contract_id_t> &k, const contract_ack_t *) {
                if (k.second == id) {
                    servers.insert(k.first);
                }
            });
            for (const server_id_t &s : servers) {
                acks.delete_key(std::make_pair(s, id));
            }
        }
        state.contracts.insert(add_contracts.begin(), add_contracts.end());
        for (const auto &region_branch : register_current_branches) {
            state.current_branches.update(region_branch.first, region_branch.second);
        }
        for (const branch_id_t &id : remove_branches) {
            state.branch_history.branches.erase(id);
        }
        state.branch_history.branches.insert(
            add_branches.branches.begin(),
            add_branches.branches.end());
    }

    /* Use `check_contract()` to make sure that `coordinate()` produced reasonable
    contracts. Its interface mirrors that of `add_contract()`. */
    cpu_contract_ids_t check_contract(
            const std::string &context,
            const char *quick_range_spec,
            const cpu_contracts_t &contracts) {
        SCOPED_TRACE("checking contract: " + context);
        key_range_t range = quick_range(quick_range_spec);
        cpu_contract_ids_t res;
        bool found[CPU_SHARDING_FACTOR];
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            found[i] = false;
        }
        for (const auto &pair : state.contracts) {
            if (pair.second.first.inner == range) {
                size_t i = get_cpu_shard_number(pair.second.first);
                EXPECT_FALSE(found[i]);
                found[i] = true;
                res.contract_ids[i] = pair.first;
                const contract_t &expect = contracts.contracts[i];
                const contract_t &actual = pair.second.second;
                EXPECT_EQ(expect.replicas, actual.replicas);
                EXPECT_EQ(expect.voters, actual.voters);
                EXPECT_EQ(expect.temp_voters, actual.temp_voters);
                EXPECT_EQ(static_cast<bool>(expect.primary),
                    static_cast<bool>(actual.primary));
                if (static_cast<bool>(expect.primary) &&
                        static_cast<bool>(actual.primary)) {
                    EXPECT_EQ(expect.primary->server, actual.primary->server);
                    EXPECT_EQ(expect.primary->hand_over, actual.primary->hand_over);
                }
            }
        }
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            EXPECT_TRUE(found[i]);
        }
        return res;
    }

    /* `check_current_branches()` makes sure that the current branch for a given
    region is set to the given set of branch IDs. */
    void check_current_branches(const cpu_branch_ids_t &branches) {
        SCOPED_TRACE("checking branches");

        bool mismatched[CPU_SHARDING_FACTOR];
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            mismatched[i] = false;
        }
        state.current_branches.visit(
            region_t(branches.range),
            [&](const region_t &reg, const branch_id_t &branch) {
                int cs = get_cpu_shard_approx_number(reg);
                /* Make sure the CPU shard matches exactly and fail otherwise. */
                EXPECT_TRUE(cpu_sharding_subspace(cs).beg == reg.beg &&
                    cpu_sharding_subspace(cs).end == reg.end);
                if (branch != branches.branch_ids[cs]) {
                    mismatched[cs] = true;
                }
            });

        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            EXPECT_FALSE(mismatched[i]);
        }
    }

    /* `check_same_contract()` checks that the same contract is still present, with the
    exact same ID. */
    void check_same_contract(const cpu_contract_ids_t &contract_ids) {
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            EXPECT_EQ(1, state.contracts.count(contract_ids.contract_ids[i]));
        }
    }

    table_raft_state_t state;
    std::set<server_id_t> all_servers;
    std::map<contract_id_t, std::map<server_id_t, contract_ack_t> > acks;
    watchable_map_var_t<std::pair<server_id_t, server_id_t>, empty_value_t> connections;
};

/* In the `AddReplica` test, we add a single replica to a table. */
TPTEST(ClusteringContractCoordinator, AddReplica) {
    server_id_t alice = generate_uuid(), billy = generate_uuid();
    coordinator_tester_t test({ alice, billy });
    test.set_config({ {"*-*", {alice}, alice} });
    cpu_branch_ids_t branch = quick_cpu_branch(
        &test.state.branch_history,
        { {"*-*", nullptr, 0} });
    test.set_current_branches(branch);
    cpu_contract_ids_t cid1 = test.add_contract("*-*",
        quick_contract_simple({alice}, alice));
    test.add_ack(alice, cid1, contract_ack_t::state_t::primary_ready);
    test.add_ack(billy, cid1, contract_ack_t::state_t::nothing);

    test.coordinate();
    test.check_same_contract(cid1);

    test.set_config({ {"*-*", {alice, billy}, alice} });

    test.coordinate();
    cpu_contract_ids_t cid2 = test.check_contract("Billy in replicas", "*-*",
        quick_contract_extra_replicas({alice}, {billy}, alice));
    test.check_current_branches(branch);

    test.add_ack(alice, cid2, contract_ack_t::state_t::primary_ready);
    test.add_ack(billy, cid2, contract_ack_t::state_t::secondary_streaming);

    test.coordinate();
    cpu_contract_ids_t cid3 = test.check_contract("Billy in temp_voters", "*-*",
        quick_contract_temp_voters({alice}, {alice, billy}, alice));
    test.check_current_branches(branch);

    test.add_ack(alice, cid3, contract_ack_t::state_t::primary_ready);
    test.add_ack(billy, cid3, contract_ack_t::state_t::secondary_streaming);

    test.coordinate();
    test.check_contract("Billy in voters", "*-*",
        quick_contract_simple({alice, billy}, alice));
    test.check_current_branches(branch);
}

/* In the `RemoveReplica` test, we remove a single replica from a table. */
TPTEST(ClusteringContractCoordinator, RemoveReplica) {
    server_id_t alice = generate_uuid(), billy = generate_uuid();
    coordinator_tester_t test({ alice, billy });
    test.set_config({ {"*-*", {alice, billy}, alice} });
    cpu_branch_ids_t branch = quick_cpu_branch(
        &test.state.branch_history,
        { {"*-*", nullptr, 0} });
    test.set_current_branches(branch);
    cpu_contract_ids_t cid1 = test.add_contract("*-*",
        quick_contract_simple({alice, billy}, alice));
    test.add_ack(alice, cid1, contract_ack_t::state_t::primary_ready);
    test.add_ack(billy, cid1, contract_ack_t::state_t::secondary_streaming);

    test.coordinate();
    test.check_same_contract(cid1);

    test.set_config({ {"*-*", {alice}, alice} });

    test.coordinate();
    cpu_contract_ids_t cid2 = test.check_contract("Billy not in temp_voters", "*-*",
        quick_contract_temp_voters({alice, billy}, {alice}, alice));
    test.check_current_branches(branch);

    test.add_ack(alice, cid2, contract_ack_t::state_t::primary_ready);
    test.add_ack(billy, cid2, contract_ack_t::state_t::secondary_streaming);

    test.coordinate();
    test.check_contract("Billy removed", "*-*",
        quick_contract_simple({alice}, alice));
    test.check_current_branches(branch);
}

/* In the `ChangePrimary` test, we move the primary from one replica to another. */
TPTEST(ClusteringContractCoordinator, ChangePrimary) {
    server_id_t alice = generate_uuid(), billy = generate_uuid();
    coordinator_tester_t test({ alice, billy });
    test.set_config({ {"*-*", {alice, billy}, alice} });
    cpu_branch_ids_t branch1 = quick_cpu_branch(
        &test.state.branch_history,
        { {"*-*", nullptr, 0} });
    test.set_current_branches(branch1);
    cpu_contract_ids_t cid1 = test.add_contract("*-*",
        quick_contract_simple({alice, billy}, alice));
    test.add_ack(alice, cid1, contract_ack_t::state_t::primary_ready);
    test.add_ack(billy, cid1, contract_ack_t::state_t::secondary_streaming);

    test.coordinate();
    test.check_same_contract(cid1);

    test.set_config({ {"*-*", {alice, billy}, billy} });

    test.coordinate();
    cpu_contract_ids_t cid2 = test.check_contract("Alice hand_over to Billy", "*-*",
        quick_contract_hand_over({alice, billy}, alice, billy));
    test.check_current_branches(branch1);

    test.add_ack(alice, cid2, contract_ack_t::state_t::primary_ready);
    test.add_ack(billy, cid2, contract_ack_t::state_t::secondary_streaming);

    test.coordinate();
    cpu_contract_ids_t cid3 = test.check_contract("No primary", "*-*",
        quick_contract_no_primary({alice, billy}));
    test.check_current_branches(branch1);

    test.add_ack(alice, cid3, contract_ack_t::state_t::secondary_need_primary,
        test.state.branch_history,
        { {"*-*", &branch1, 123} });
    test.add_ack(billy, cid3, contract_ack_t::state_t::secondary_need_primary,
        test.state.branch_history,
        { {"*-*", &branch1, 123} });

    test.coordinate();
    cpu_contract_ids_t cid4 = test.check_contract("Billy primary; old branch", "*-*",
        quick_contract_simple({alice, billy}, billy));
    test.check_current_branches(branch1);

    branch_history_t billy_branch_history = test.state.branch_history;
    cpu_branch_ids_t branch2 = quick_cpu_branch(
        &billy_branch_history,
        { {"*-*", &branch1, 123} });
    test.add_ack(alice, cid4, contract_ack_t::state_t::secondary_need_primary,
        test.state.branch_history,
        { {"*-*", &branch1, 123} });
    test.add_ack(billy, cid4, contract_ack_t::state_t::primary_need_branch,
        billy_branch_history, &branch2);

    test.coordinate();
    test.check_contract("Billy primary; new branch", "*-*",
        quick_contract_simple({alice, billy}, billy));
    test.check_current_branches(branch2);
}

/* In the `Split` test, we break a shard into two sub-shards. */
TPTEST(ClusteringContractCoordinator, Split) {
    server_id_t alice = generate_uuid(), billy = generate_uuid();
    coordinator_tester_t test({ alice, billy });
    test.set_config({ {"*-*", {alice}, alice} });
    cpu_branch_ids_t branch1 = quick_cpu_branch(
        &test.state.branch_history,
        { {"*-*", nullptr, 0} });
    test.set_current_branches(branch1);
    cpu_contract_ids_t cid1 = test.add_contract("*-*",
        quick_contract_simple({alice}, alice));
    test.add_ack(alice, cid1, contract_ack_t::state_t::primary_ready);
    test.add_ack(billy, cid1, contract_ack_t::state_t::nothing);

    test.coordinate();
    test.check_same_contract(cid1);

    test.set_config({ {"*-M", {alice}, alice}, {"N-*", {billy}, billy} });

    test.coordinate();
    cpu_contract_ids_t cid2ABC = test.check_contract("L: Alice remains primary", "*-M",
        quick_contract_simple({alice}, alice));
    cpu_contract_ids_t cid2DE = test.check_contract("R: Billy becomes replica", "N-*",
        quick_contract_extra_replicas({alice}, {billy}, alice));
    test.check_current_branches(branch1);

    branch_history_t alice_branch_history = test.state.branch_history;
    cpu_branch_ids_t branch2ABC = quick_cpu_branch(
        &alice_branch_history,
        { {"*-M", &branch1, 123} });
    cpu_branch_ids_t branch2DE = quick_cpu_branch(
        &alice_branch_history,
        { {"N-*", &branch1, 123} });
    test.add_ack(alice, cid2ABC, contract_ack_t::state_t::primary_need_branch,
        alice_branch_history, &branch2ABC);
    test.add_ack(billy, cid2ABC, contract_ack_t::state_t::nothing);
    test.add_ack(alice, cid2DE, contract_ack_t::state_t::primary_need_branch,
        alice_branch_history, &branch2DE);
    test.add_ack(billy, cid2DE, contract_ack_t::state_t::secondary_need_primary,
        branch_history_t(),
        { {"N-*", nullptr, 0} });

    test.coordinate();
    cpu_contract_ids_t cid3ABC = test.check_contract("L: Alice gets branch ID", "*-M",
        quick_contract_simple({alice}, alice));
    test.check_current_branches(branch2ABC);
    cpu_contract_ids_t cid3DE = test.check_contract("R: Alice gets branch ID", "N-*",
        quick_contract_extra_replicas({alice}, {billy}, alice));
    test.check_current_branches(branch2DE);

    test.add_ack(alice, cid3ABC, contract_ack_t::state_t::primary_ready);
    test.add_ack(billy, cid3ABC, contract_ack_t::state_t::nothing);
    test.add_ack(alice, cid3DE, contract_ack_t::state_t::primary_ready);
    test.add_ack(billy, cid3DE, contract_ack_t::state_t::secondary_streaming);

    test.coordinate();
    test.check_same_contract(cid3ABC);
    cpu_contract_ids_t cid4DE = test.check_contract("L: Hand over", "N-*",
        quick_contract_temp_voters_hand_over(
            {alice}, {billy}, alice, billy));
    test.check_current_branches(branch2DE);

    test.add_ack(alice, cid4DE, contract_ack_t::state_t::primary_ready);
    test.add_ack(billy, cid4DE, contract_ack_t::state_t::secondary_streaming);

    test.coordinate();
    test.check_same_contract(cid3ABC);
    cpu_contract_ids_t cid5DE = test.check_contract("L: No primary", "N-*",
        quick_contract_no_primary({billy}));
    test.check_current_branches(branch2DE);

    test.add_ack(alice, cid5DE, contract_ack_t::state_t::nothing);
    test.add_ack(billy, cid5DE, contract_ack_t::state_t::secondary_need_primary,
        test.state.branch_history,
        { {"N-*", &branch2DE, 456 } });

    test.coordinate();
    test.check_same_contract(cid3ABC);
    cpu_contract_ids_t cid6DE = test.check_contract("L: Billy primary old branch", "N-*",
        quick_contract_simple({billy}, billy));
    test.check_current_branches(branch2DE);

    branch_history_t billy_branch_history = test.state.branch_history;
    cpu_branch_ids_t branch3DE = quick_cpu_branch(
        &billy_branch_history,
        { {"N-*", &branch2DE, 456} });
    test.add_ack(alice, cid6DE, contract_ack_t::state_t::nothing);
    test.add_ack(billy, cid6DE, contract_ack_t::state_t::primary_need_branch,
        billy_branch_history, &branch3DE);

    test.coordinate();
    test.check_same_contract(cid3ABC);
    test.check_contract("L: Billy primary new branch", "N-*",
        quick_contract_simple({billy}, billy));
    test.check_current_branches(branch3DE);
}

/* In the `Failover` test, we test that a new primary will be elected if the old primary
fails. */
TPTEST(ClusteringContractCoordinator, Failover) {
    server_id_t alice = generate_uuid(),
                billy = generate_uuid(),
                carol = generate_uuid();
    coordinator_tester_t test({ alice, billy, carol });
    test.set_config({ {"*-*", {alice, billy, carol}, alice} });
    cpu_branch_ids_t branch1 = quick_cpu_branch(
        &test.state.branch_history,
        { {"*-*", nullptr, 0} });
    cpu_contract_ids_t cid1 = test.add_contract("*-*",
        quick_contract_simple({alice, billy, carol}, alice));
    test.set_current_branches(branch1);
    test.add_ack(alice, cid1, contract_ack_t::state_t::primary_ready);
    test.add_ack(billy, cid1, contract_ack_t::state_t::secondary_streaming);
    test.add_ack(carol, cid1, contract_ack_t::state_t::secondary_streaming);

    test.coordinate();
    test.check_same_contract(cid1);

    /* Report that the primary has failed, but initially indicate that both of the
    secondaries can still see it; nothing will happen. */
    test.set_visibility(alice, false);
    test.set_visibility(billy, alice, true);
    test.set_visibility(carol, alice, true);
    test.remove_ack(alice, cid1);
    test.add_ack(billy, cid1, contract_ack_t::state_t::secondary_need_primary,
        test.state.branch_history,
        { {"*-*", &branch1, 100} });
    test.add_ack(carol, cid1, contract_ack_t::state_t::secondary_need_primary,
        test.state.branch_history,
        { {"*-*", &branch1, 101} });

    test.coordinate();
    test.check_same_contract(cid1);

    /* OK, now try again with both secondaries reporting the primary is disconnected. */
    test.set_visibility(alice, false);
    test.add_ack(billy, cid1, contract_ack_t::state_t::secondary_need_primary,
        test.state.branch_history,
        { {"*-*", &branch1, 100} });
    test.add_ack(carol, cid1, contract_ack_t::state_t::secondary_need_primary,
        test.state.branch_history,
        { {"*-*", &branch1, 101} });

    test.coordinate();
    test.check_contract("Failover", "*-*",
        quick_contract_no_primary({alice, billy, carol}));
    test.check_current_branches(branch1);
}

/* In the `FailoverSplit` test, we test a corner case where different servers are
eligile to be primary for different parts of the new key-space. */
TPTEST(ClusteringContractCoordinator, FailoverSplit) {
    server_id_t alice = generate_uuid(),
                billy = generate_uuid(),
                carol = generate_uuid();
    coordinator_tester_t test({ alice, billy, carol });
    test.set_config({ {"*-*", {alice, billy, carol}, alice} });
    cpu_branch_ids_t branch1 = quick_cpu_branch(
        &test.state.branch_history,
        { {"*-*", nullptr, 0} });
    cpu_contract_ids_t cid1 = test.add_contract("*-*",
        quick_contract_simple({alice, billy, carol}, alice));
    test.set_current_branches(branch1);
    test.add_ack(alice, cid1, contract_ack_t::state_t::primary_ready);
    test.add_ack(billy, cid1, contract_ack_t::state_t::secondary_streaming);
    test.add_ack(carol, cid1, contract_ack_t::state_t::secondary_streaming);

    test.coordinate();
    test.check_same_contract(cid1);

    test.remove_ack(alice, cid1);
    test.set_visibility(alice, false);
    test.add_ack(billy, cid1, contract_ack_t::state_t::secondary_need_primary,
        test.state.branch_history,
        { {"*-*", &branch1, 100} });
    test.add_ack(carol, cid1, contract_ack_t::state_t::secondary_need_primary,
        test.state.branch_history,
        { {"*-M", &branch1, 101}, {"N-*", &branch1, 99} });

    test.coordinate();
    cpu_contract_ids_t cid2 = test.check_contract("No primary", "*-*",
        quick_contract_no_primary({alice, billy, carol}));
    test.check_current_branches(branch1);

    test.add_ack(billy, cid2, contract_ack_t::state_t::secondary_need_primary,
        test.state.branch_history,
        { {"*-M", &branch1, 100}, {"N-*", &branch1, 100} });
    test.add_ack(carol, cid2, contract_ack_t::state_t::secondary_need_primary,
        test.state.branch_history,
        { {"*-M", &branch1, 101}, {"N-*", &branch1, 99 } });

    test.coordinate();
    test.check_contract("L: Failover", "*-M",
        quick_contract_simple({alice, billy, carol}, carol));
    test.check_contract("R: Failover", "N-*",
        quick_contract_simple({alice, billy, carol}, billy));
    test.check_current_branches(branch1);
}

} /* namespace unittest */

