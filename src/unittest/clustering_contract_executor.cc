// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "clustering/table_contract/executor.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_contract_utils.hpp"
#include "unittest/clustering_utils.hpp"

namespace unittest {

class executor_tester_context_t {
public:
    executor_tester_context_t() :
        io_backender(file_direct_io_mode_t::buffered_desired),
        published_state(state)
        { }
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
    void remove_contract(const cpu_contract_ids_t &ids) {
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            state.contracts.erase(ids.contract_ids[i]);
        }
    }
    void publish() {
        published_state.set_value_no_equals(state);
    }
    table_raft_state_t state;
private:
    friend class executor_tester_t;
    simple_mailbox_cluster_t cluster;
    io_backender_t io_backender;
    backfill_throttler_t backfill_throttler;
    watchable_map_var_t<
        std::pair<server_id_t, branch_id_t>,
        contract_execution_bcard_t> contract_execution_bcards;
    watchable_variable_t<table_raft_state_t> published_state;
};

class executor_tester_files_t {
public:
    executor_tester_files_t(const server_id_t &_server_id) : server_id(_server_id) {
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            /* RSI(raft): Spread stores across threads */
            stores[i].init(new mock_store_t(binary_blob_t(version_t::zero())));
            multistore.shards[i] = stores[i].get();
        }
    }
private:
    friend class executor_tester_t;
    server_id_t server_id;
    scoped_ptr_t<mock_store_t> stores[CPU_SHARDING_FACTOR];
    multistore_ptr_t multistore;
    in_memory_branch_history_manager_t branch_history_manager;
};

class executor_tester_t {
public:
    executor_tester_t(
            executor_tester_context_t *_context,
            executor_tester_files_t *_files) :
        context(_context), files(_files)
    {
        executor.init(new contract_executor_t(
            files->server_id,
            context->cluster.get_mailbox_manager(),
            context->published_state.get_watchable(),
            &context->contract_execution_bcards,
            &files->multistore,
            &files->branch_history_manager,
            base_path_t("."),
            &context->io_backender,
            &context->backfill_throttler,
            &get_global_perfmon_collection()));

        /* Copy our contract execution bcards into the context's map so that other
        `executor_tester_t`s can see them. */
        bcard_copier.init(new watchable_map_t<
                std::pair<server_id_t, branch_id_t>,
                contract_execution_bcard_t>::all_subs_t(
            executor->get_local_contract_execution_bcards(),
            [this](const std::pair<server_id_t, branch_id_t> &key,
                    const contract_execution_bcard_t *value) {
                if (value != nullptr) {
                    context->contract_execution_bcards.set_key_no_equals(key, *value);
                } else {
                    context->contract_execution_bcards.delete_key(key);
                }
            },
            true));
    }

    ~executor_tester_t() {
        /* Remove our contract execution bcards from the context's map */
        bcard_copier.reset();
        std::set<branch_id_t> to_delete;
        context->contract_execution_bcards.read_all(
            [&](const std::pair<server_id_t, branch_id_t> &key,
                    const contract_execution_bcard_t *) {
                to_delete.insert(key.second);
            });
        for (const branch_id_t &b : to_delete) {
            context->contract_execution_bcards.delete_key(
                std::make_pair(files->server_id, b));
        }
    }

    void write_primary(const std::string &key, const std::string &value) {
        scoped_ptr_t<primary_query_client_t> primary = find_primary(key);
        ASSERT_TRUE(primary.has());
        fifo_enforcer_sink_t::exit_write_t token;
        primary->new_write_token(&token);
        write_response_t response;
        cond_t non_interruptor;
        primary->write(
            mock_overwrite(key, value),
            &response,
            order_token_t::ignore,
            &token,
            &non_interruptor);
    }

    void read_primary(const std::string &key, const std::string &expect) {
        scoped_ptr_t<primary_query_client_t> primary = find_primary(key);
        ASSERT_TRUE(primary.has());
        fifo_enforcer_sink_t::exit_read_t token;
        primary->new_read_token(&token);
        read_response_t response;
        cond_t non_interruptor;
        primary->read(
            mock_read(key),
            &response,
            order_token_t::ignore,
            &token,
            &non_interruptor);
        EXPECT_EQ(expect, mock_parse_read_response(response));
    }

    void read_store(const std::string &key, const std::string &expect) {
        store_key_t key2(key);
        uint64_t hash = hash_region_hasher(key2.contents(), key2.size());
        size_t cpu_shard = hash / (HASH_REGION_HASH_SIZE / CPU_SHARDING_FACTOR);
        std::string value = mock_lookup(files->stores[cpu_shard].get(), key);
        EXPECT_EQ(expect, value);
    }

    void check_acks(
            const cpu_contract_ids_t &contract_ids,
            contract_ack_t::state_t state) {
        signal_timer_t timeout;
        timeout.start(1000);
        try {
            for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
                executor->get_acks()->run_key_until_satisfied(
                    std::make_pair(files->server_id, contract_ids.contract_ids[i]),
                    [&](const contract_ack_t *ack) {
                        return ack != nullptr && state == ack->state;
                    }, &timeout);
            }
        } catch (const interrupted_exc_t &) {
            ADD_FAILURE() << "contract not acked as expected within 1000ms";
        }
    }

    void check_acks(
            const cpu_contract_ids_t &contract_ids,
            contract_ack_t::state_t state,
            branch_history_t *branch_history_out,
            cpu_branch_ids_t *branch_out) {
        guarantee(state == contract_ack_t::state_t::primary_need_branch);
        branch_out->range = contract_ids.range;
        signal_timer_t timeout;
        timeout.start(1000);
        try {
            for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
                executor->get_acks()->run_key_until_satisfied(
                    std::make_pair(files->server_id, contract_ids.contract_ids[i]),
                    [&](const contract_ack_t *ack) {
                        if (ack == nullptr || ack->state != state) {
                            return false;
                        }
                        guarantee(static_cast<bool>(ack->branch));
                        branch_history_out->branches.insert(
                            ack->branch_history.branches.begin(),
                            ack->branch_history.branches.end());
                        branch_out->branch_ids[i] = *ack->branch;
                        return true;
                    }, &timeout);
            }
        } catch (const interrupted_exc_t &) {
            ADD_FAILURE() << "contract not acked as expected within 1000ms";
        }
    }

private:
    scoped_ptr_t<primary_query_client_t> find_primary(const std::string &key) {
        boost::optional<primary_query_bcard_t> chosen_bcard;
        executor->get_local_table_query_bcards()->read_all(
            [&](const uuid_u &, const table_query_bcard_t *bcard) {
                if (region_contains_key(bcard->region, store_key_t(key)) &&
                        static_cast<bool>(bcard->primary)) {
                    EXPECT_FALSE(static_cast<bool>(chosen_bcard));
                    chosen_bcard = boost::make_optional(*bcard->primary);
                }
            });
        if (!static_cast<bool>(chosen_bcard)) {
            return scoped_ptr_t<primary_query_client_t>();
        }
        cond_t non_interruptor;
        return make_scoped<primary_query_client_t>(
            context->cluster.get_mailbox_manager(),
            *chosen_bcard,
            &non_interruptor);
    }

    executor_tester_context_t * const context;
    executor_tester_files_t * const files;
    scoped_ptr_t<contract_executor_t> executor;
    scoped_ptr_t<watchable_map_t<
        std::pair<server_id_t, branch_id_t>,
        contract_execution_bcard_t>::all_subs_t> bcard_copier;
};

TPTEST(ClusteringContractExecutor, SimpleTests) {
    server_id_t alice = generate_uuid(), billy = generate_uuid();

    executor_tester_context_t context;
    cpu_contract_ids_t cid1 = context.add_contract("*-*",
        quick_contract_simple({alice}, alice, nullptr));
    context.publish();

    executor_tester_files_t alice_files(alice);
    executor_tester_t alice_exec(&context, &alice_files);

    cpu_branch_ids_t branch1;
    alice_exec.check_acks(cid1, contract_ack_t::state_t::primary_need_branch,
        &context.state.branch_history, &branch1);

    context.remove_contract(cid1);
    cpu_contract_ids_t cid2 = context.add_contract("*-*",
        quick_contract_simple({alice}, alice, &branch1));
    context.publish();

    alice_exec.check_acks(cid2, contract_ack_t::state_t::primary_ready);
    alice_exec.write_primary("hello", "world");
    alice_exec.read_primary("hello", "world");

    executor_tester_files_t billy_files(billy);
    executor_tester_t billy_exec(&context, &billy_files);

    context.remove_contract(cid2);
    cpu_contract_ids_t cid3 = context.add_contract("*-*",
        quick_contract_extra_replicas({alice}, {billy}, alice, &branch1));
    context.publish();

    alice_exec.check_acks(cid3, contract_ack_t::state_t::primary_ready);
    billy_exec.check_acks(cid3, contract_ack_t::state_t::secondary_streaming);
    alice_exec.write_primary("good", "morning");
    billy_exec.read_store("good", "morning");

    context.remove_contract(cid3);
    cpu_contract_ids_t cid4 = context.add_contract("*-*",
        quick_contract_temp_voters({alice}, {alice, billy}, alice, &branch1));
    context.publish();

    alice_exec.check_acks(cid4, contract_ack_t::state_t::primary_ready);
    billy_exec.check_acks(cid4, contract_ack_t::state_t::secondary_streaming);

    context.remove_contract(cid4);
    cpu_contract_ids_t cid5 = context.add_contract("*-*",
        quick_contract_simple({alice, billy}, alice, &branch1));
    context.publish();

    alice_exec.check_acks(cid5, contract_ack_t::state_t::primary_ready);
    billy_exec.check_acks(cid5, contract_ack_t::state_t::secondary_streaming);
    alice_exec.write_primary("just", "made it");

    context.remove_contract(cid5);
    cpu_contract_ids_t cid6 = context.add_contract("*-*",
        quick_contract_hand_over({alice, billy}, alice, billy, &branch1));
    context.publish();

    alice_exec.check_acks(cid6, contract_ack_t::state_t::primary_ready);
    billy_exec.read_store("just", "made it");
    billy_exec.check_acks(cid6, contract_ack_t::state_t::secondary_streaming);

    context.remove_contract(cid6);
    cpu_contract_ids_t cid7 = context.add_contract("*-*",
        quick_contract_no_primary({alice, billy}, &branch1));
    context.publish();

    alice_exec.check_acks(cid7, contract_ack_t::state_t::secondary_need_primary);
    billy_exec.check_acks(cid7, contract_ack_t::state_t::secondary_need_primary);

    context.remove_contract(cid7);
    cpu_contract_ids_t cid8 = context.add_contract("*-*",
        quick_contract_simple({alice, billy}, billy, &branch1));
    context.publish();

    alice_exec.check_acks(cid8, contract_ack_t::state_t::secondary_need_primary);
    cpu_branch_ids_t branch2;
    billy_exec.check_acks(cid8, contract_ack_t::state_t::primary_need_branch,
        &context.state.branch_history, &branch2);

    context.remove_contract(cid8);
    cpu_contract_ids_t cid9 = context.add_contract("*-*",
        quick_contract_simple({alice, billy}, billy, &branch2));
    context.publish();

    alice_exec.check_acks(cid9, contract_ack_t::state_t::secondary_streaming);
    billy_exec.check_acks(cid9, contract_ack_t::state_t::primary_ready);

    context.remove_contract(cid9);
    cpu_contract_ids_t cid10 = context.add_contract("*-*",
        quick_contract_temp_voters({alice, billy}, {billy}, billy, &branch2));
    context.publish();

    alice_exec.check_acks(cid10, contract_ack_t::state_t::secondary_streaming);
    billy_exec.check_acks(cid10, contract_ack_t::state_t::primary_ready);

    context.remove_contract(cid10);
    cpu_contract_ids_t cid11 = context.add_contract("*-*",
        quick_contract_simple({billy}, billy, &branch2));
    context.publish();

    alice_exec.check_acks(cid11, contract_ack_t::state_t::nothing);
    billy_exec.check_acks(cid11, contract_ack_t::state_t::primary_ready);
    alice_exec.read_store("hello", "");
    alice_exec.read_store("good", "");
    alice_exec.read_store("just", "");
    billy_exec.read_primary("hello", "world");
    billy_exec.read_primary("good", "morning");
    billy_exec.read_primary("just", "made it");
}

/* RSI(raft): More tests */

} /* namespace unittest */

