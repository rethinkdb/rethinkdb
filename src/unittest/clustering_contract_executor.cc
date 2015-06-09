// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "clustering/table_contract/executor/executor.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_contract_utils.hpp"
#include "unittest/clustering_utils.hpp"

namespace unittest {

/* This file contains tests for the `contract_executor_t`. Here's a general outline of
how to use it:

- Set up an `executor_tester_context_t`.
- Put a contract in the `executor_tester_context_t` by calling `add_contract()`, then
    call `publish()`
- Set up some number of `executor_tester_files_t`s and `executor_tester_t`s.
- Call `executor_tester_t::check_acks()` to see that they sent the proper acknowledgement
- Use `remove_contract()` and `add_contract()` to change the contract again, then call
    `publish()` to send it out to the `executor_tester_t`s.
- Repeat the last two steps to simulate a multi-step exchange between the executor and
    coordinator
- Optionally, use the `read_primary()`, `write_primary()`, and `read_store()` methods on
    the `executor_tester_t`, and the class `write_generator_t`, to generate test traffic
    to the executors. */

class executor_tester_context_t;
class executor_tester_files_t;
class executor_tester_t;

/* `executor_tester_context_t` acts as a fake coordinator for a collection of
`executor_tester_t`s. It also provides some pieces of clustering infrastructure to let
them communicate with each other. */
class executor_tester_context_t {
public:
    executor_tester_context_t() :
        io_backender(file_direct_io_mode_t::buffered_desired),
        published_state(state)
        { }
    /* Note that when you create or destroy contracts with `add_contract()` and
    `remove_contract()`, the `executor_tester_t`s will not see the changes until you call
    `publish()`! This is to make sure they never see broken intermediate states. */
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
    void set_current_branches(const cpu_branch_ids_t &branches) {
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            region_t reg = cpu_sharding_subspace(i);
            reg.inner = branches.range;
            state.current_branches.update(reg, branches.branch_ids[i]);
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

/* `executor_tester_files_t` represents a set of on-disk files for an
`executor_tester_t`. It's separate from `executor_tester_t` so that you can simulate
shutting down and restarting an `executor_tester_t`. */
class executor_tester_files_t : public multistore_ptr_t {
public:
    explicit executor_tester_files_t(const server_id_t &_server_id) :
            server_id(_server_id) {
        int next_thread = 0;
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            stores[i].init(new mock_store_t(binary_blob_t(version_t::zero())));
            stores[i]->rethread(threadnum_t(next_thread));
            next_thread = (next_thread + 1) % get_num_threads();
        }
    }
    ~executor_tester_files_t() {
        for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
            stores[i]->rethread(home_thread());
        }
    }
    branch_history_manager_t *get_branch_history_manager() {
        return &branch_history_manager;
    }
    store_view_t *get_cpu_sharded_store(size_t i) {
        return stores[i].get();
    }
    store_t *get_underlying_store(UNUSED size_t i) {
        crash("not implemented for this unit test");
    }
private:
    friend class executor_tester_t;
    server_id_t server_id;
    scoped_ptr_t<mock_store_t> stores[CPU_SHARDING_FACTOR];
    in_memory_branch_history_manager_t branch_history_manager;
};

/* `executor_tester_t` is the star of the show: a class that wraps `contract_executor_t`.
It listens for contracts from the `executor_tester_context_t` and forwards them to its
`contract_executor_t`; then it allows checking the acks via `check_acks()`. */
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
            files,
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
            initial_call_t::YES));
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

    /* `write_primary()` performs a write through a primary hosted on the
    `contract_executor_t`, if it can find such a primary. If anything goes wrong, it
    throws `cannot_perform_query_exc_t`. */
    void write_primary(const std::string &key, const std::string &value) {
        primary_finder_t primary_finder(this, key);
        fifo_enforcer_sink_t::exit_write_t token;
        primary_finder.get_client()->new_write_token(&token);
        write_response_t response;
        try {
            primary_finder.get_client()->write(
                mock_overwrite(key, value),
                &response,
                order_token_t::ignore,
                &token,
                primary_finder.get_disconnect_signal());
        } catch (const interrupted_exc_t &) {
            throw cannot_perform_query_exc_t("lost contact with primary");
        }
    }

    /* `read_primary()` performs a read through a primary hosted on the
    `contract_executor_t`, if it can find such a primary. If anything goes wrong, it
    throws `cannot_perform_query_exc_t`. Instead of returning the result of the read, it
    compares it to `expect`. */
    void read_primary(const std::string &key, const std::string &expect) {
        primary_finder_t primary_finder(this, key);
        fifo_enforcer_sink_t::exit_read_t token;
        primary_finder.get_client()->new_read_token(&token);
        read_response_t response;
        try {
            primary_finder.get_client()->read(
                mock_read(key),
                &response,
                order_token_t::ignore,
                &token,
                primary_finder.get_disconnect_signal());
        } catch (const interrupted_exc_t &) {
            throw cannot_perform_query_exc_t("lost contact with primary");
        }
        EXPECT_EQ(expect, mock_parse_read_response(response));
    }

    /* `read_store()` is like `read_primary()` except that it goes directly to the
    storage layer instead of routing the read through the primary. So it works on any
    server, even if it's not a primary. */
    void read_store(const std::string &key, const std::string &expect) {
        store_key_t key2(key);
        uint64_t hash = hash_region_hasher(key2);
        size_t cpu_shard = hash / (HASH_REGION_HASH_SIZE / CPU_SHARDING_FACTOR);
        std::string value;
        {
            on_thread_t thread_switcher(files->stores[cpu_shard]->home_thread());
            value = mock_lookup(files->stores[cpu_shard].get(), key);
        }
        EXPECT_EQ(expect, value);
    }

    /* `check_acks()` expects to find that the `contract_executor_t` has sent the given
    the given `contract_ack_t::state_t` in response for the given contract. It waits up
    to a second for this to be true, and fails if it is not. */
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

    /* This form of `check_acks` is only for use with the `primary_need_branch` state. It
    returns the branch IDs after verifying the acks. */
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
    /* `primary_finder_t` looks for a `primary_query_bcard_t` published by the
    `contract_executor_t` and sets up a `primary_query_client_t` for it. It also monitors
    for the bcard being destroyed. */
    class primary_finder_t {
    public:
        primary_finder_t(executor_tester_t *p, const std::string &key) :
                parent(p) {
            bool found = false;
            uuid_u bcard_uuid;
            primary_query_bcard_t bcard;
            parent->executor->get_local_table_query_bcards()->read_all(
                [&](const uuid_u &u, const table_query_bcard_t *b) {
                    if (region_contains_key(b->region, store_key_t(key)) &&
                            static_cast<bool>(b->primary)) {
                        EXPECT_FALSE(found);
                        bcard_uuid = u;
                        bcard = *b->primary;
                        found = true;
                    }
                });
            if (found) {
                disconnect_watcher = make_scoped<
                        watchable_map_t<uuid_u, table_query_bcard_t>::key_subs_t>(
                    parent->executor->get_local_table_query_bcards(),
                    bcard_uuid,
                    [this](const table_query_bcard_t *b) {
                        if (b == nullptr) {
                            disconnect.pulse_if_not_already_pulsed();
                        }
                    },
                    initial_call_t::YES);
                try {
                    query_client = make_scoped<primary_query_client_t>(
                        parent->context->cluster.get_mailbox_manager(),
                        bcard,
                        &disconnect);
                } catch (const interrupted_exc_t &) {
                     throw cannot_perform_query_exc_t("lost contact with primary");
                }
            } else {
                throw cannot_perform_query_exc_t("no primary found on this executor");
            }
        }
        signal_t *get_disconnect_signal() {
            return &disconnect;
        }
        primary_query_client_t *get_client() {
            return query_client.get();
        }
    private:
        executor_tester_t *parent;
        cond_t disconnect;
        scoped_ptr_t<watchable_map_t<uuid_u, table_query_bcard_t>::key_subs_t>
            disconnect_watcher;
        scoped_ptr_t<primary_query_client_t> query_client;
    };

    executor_tester_context_t * const context;
    executor_tester_files_t * const files;
    scoped_ptr_t<contract_executor_t> executor;
    scoped_ptr_t<watchable_map_t<
        std::pair<server_id_t, branch_id_t>,
        contract_execution_bcard_t>::all_subs_t> bcard_copier;
};

/* `write_generator_t` sends a continuous stream of writes to the given
`executor_tester_t` via its `write_primary()` method. If the writes fail for any reason,
it silently ignores the error. Call `verify()` to make sure that every successful write
is present on the given `executor_tester_t`. */
class write_generator_t {
public:
    explicit write_generator_t(executor_tester_t *t) :
            target(t), next_int(0), ack_target_change(nullptr) {
        coro_t::spawn_sometime(std::bind(&write_generator_t::run, this, drainer.lock()));
    }
    /* `change_target()` changes where writes are sent to. It will block until the change
    is finished; this is useful if you intend to destroy the old target. You can also
    pass `nullptr` to stop sending writes. */
    void change_target(executor_tester_t *t) {
        target = t;
        cond_t cond;
        assignment_sentry_t<cond_t *> sentry(&ack_target_change, &cond);
        cond.wait_lazily_unordered();
    }
    void check_writes_work() {
        std::string key = strprintf("key%d", next_int);
        std::string value = strprintf("value%d", next_int);
        ++next_int;
        guarantee(target != nullptr);
        try {
            target->write_primary(key, value);
            acked.insert(std::make_pair(key, value));
        } catch (const cannot_perform_query_exc_t &) {
            ADD_FAILURE() << "expected writes to work at this point";
        }
    }
    void verify(executor_tester_t *reader) {
        std::map<std::string, std::string> copy = acked;
        for (const auto &pair : copy) {
            reader->read_store(pair.first, pair.second);
        }
    }
private:
    void run(auto_drainer_t::lock_t keepalive) {
        try {
            while (!keepalive.get_drain_signal()->is_pulsed()) {
                std::string key = strprintf("key%d", next_int);
                std::string value = strprintf("value%d", next_int);
                ++next_int;
                if (target != nullptr) {
                    try {
                        target->write_primary(key, value);
                        acked.insert(std::make_pair(key, value));
                    } catch (const cannot_perform_query_exc_t &) {
                        /* do nothing */
                    }
                }
                if (ack_target_change != nullptr) {
                    ack_target_change->pulse_if_not_already_pulsed();
                }
                nap(10, keepalive.get_drain_signal());
            }
        } catch (const interrupted_exc_t &) {
            /* ignore */
        }
    }
    executor_tester_t *target;
    int next_int;
    cond_t *ack_target_change;
    std::map<std::string, std::string> acked;
    auto_drainer_t drainer;
};

TPTEST(ClusteringContractExecutor, SimpleTests) {
    server_id_t alice = generate_uuid(), billy = generate_uuid();

    /* Bring up the primary, `alice`, with no secondaries */

    executor_tester_context_t context;
    cpu_contract_ids_t cid1 = context.add_contract("*-*",
        quick_contract_simple({alice}, alice));
    context.publish();

    executor_tester_files_t alice_files(alice);
    executor_tester_t alice_exec(&context, &alice_files);

    cpu_branch_ids_t branch1;
    alice_exec.check_acks(cid1, contract_ack_t::state_t::primary_need_branch,
        &context.state.branch_history, &branch1);

    context.remove_contract(cid1);
    cpu_contract_ids_t cid2 = context.add_contract("*-*",
        quick_contract_simple({alice}, alice));
    context.set_current_branches(branch1);
    context.publish();

    alice_exec.check_acks(cid2, contract_ack_t::state_t::primary_ready);
    alice_exec.write_primary("hello", "world");
    alice_exec.read_primary("hello", "world");

    /* Add a secondary, `billy` */

    executor_tester_files_t billy_files(billy);
    executor_tester_t billy_exec(&context, &billy_files);

    context.remove_contract(cid2);
    cpu_contract_ids_t cid3 = context.add_contract("*-*",
        quick_contract_extra_replicas({alice}, {billy}, alice));
    context.set_current_branches(branch1);
    context.publish();

    alice_exec.check_acks(cid3, contract_ack_t::state_t::primary_ready);
    billy_exec.check_acks(cid3, contract_ack_t::state_t::secondary_streaming);

    write_generator_t write_generator(&alice_exec);
    write_generator.check_writes_work();

    context.remove_contract(cid3);
    cpu_contract_ids_t cid4 = context.add_contract("*-*",
        quick_contract_temp_voters({alice}, {alice, billy}, alice));
    context.set_current_branches(branch1);
    context.publish();

    alice_exec.check_acks(cid4, contract_ack_t::state_t::primary_ready);
    billy_exec.check_acks(cid4, contract_ack_t::state_t::secondary_streaming);
    write_generator.check_writes_work();

    context.remove_contract(cid4);
    cpu_contract_ids_t cid5 = context.add_contract("*-*",
        quick_contract_simple({alice, billy}, alice));
    context.set_current_branches(branch1);
    context.publish();

    alice_exec.check_acks(cid5, contract_ack_t::state_t::primary_ready);
    billy_exec.check_acks(cid5, contract_ack_t::state_t::secondary_streaming);
    write_generator.check_writes_work();

    /* Make `billy` the primary instead of `alice` */

    context.remove_contract(cid5);
    cpu_contract_ids_t cid6 = context.add_contract("*-*",
        quick_contract_hand_over({alice, billy}, alice, billy));
    context.set_current_branches(branch1);
    context.publish();

    alice_exec.check_acks(cid6, contract_ack_t::state_t::primary_ready);
    billy_exec.check_acks(cid6, contract_ack_t::state_t::secondary_streaming);

    context.remove_contract(cid6);
    cpu_contract_ids_t cid7 = context.add_contract("*-*",
        quick_contract_no_primary({alice, billy}));
    context.set_current_branches(branch1);
    context.publish();
    write_generator.change_target(&billy_exec);

    alice_exec.check_acks(cid7, contract_ack_t::state_t::secondary_need_primary);
    billy_exec.check_acks(cid7, contract_ack_t::state_t::secondary_need_primary);
    write_generator.verify(&alice_exec);
    write_generator.verify(&billy_exec);

    context.remove_contract(cid7);
    cpu_contract_ids_t cid8 = context.add_contract("*-*",
        quick_contract_simple({alice, billy}, billy));
    context.set_current_branches(branch1);
    context.publish();

    alice_exec.check_acks(cid8, contract_ack_t::state_t::secondary_need_primary);
    cpu_branch_ids_t branch2;
    billy_exec.check_acks(cid8, contract_ack_t::state_t::primary_need_branch,
        &context.state.branch_history, &branch2);

    context.remove_contract(cid8);
    cpu_contract_ids_t cid9 = context.add_contract("*-*",
        quick_contract_simple({alice, billy}, billy));
    context.set_current_branches(branch2);
    context.publish();

    alice_exec.check_acks(cid9, contract_ack_t::state_t::secondary_streaming);
    billy_exec.check_acks(cid9, contract_ack_t::state_t::primary_ready);
    write_generator.check_writes_work();

    context.remove_contract(cid9);
    cpu_contract_ids_t cid10 = context.add_contract("*-*",
        quick_contract_temp_voters({alice, billy}, {billy}, billy));
    context.set_current_branches(branch2);
    context.publish();

    alice_exec.check_acks(cid10, contract_ack_t::state_t::secondary_streaming);
    billy_exec.check_acks(cid10, contract_ack_t::state_t::primary_ready);
    write_generator.check_writes_work();
    write_generator.verify(&alice_exec);
    write_generator.verify(&billy_exec);

    /* Remove `alice` */

    context.remove_contract(cid10);
    cpu_contract_ids_t cid11 = context.add_contract("*-*",
        quick_contract_simple({billy}, billy));
    context.set_current_branches(branch2);
    context.publish();

    alice_exec.check_acks(cid11, contract_ack_t::state_t::nothing);
    billy_exec.check_acks(cid11, contract_ack_t::state_t::primary_ready);
    write_generator.check_writes_work();
    write_generator.verify(&billy_exec);

    /* We want to make sure that `alice` erased its data. But it will ack `nothing`
    before it actually erases the data. So we wait 100ms before checking for the data to
    be erased. */
    nap(100);
    alice_exec.read_store("hello", "");   /* confirm that data was erased */
}

TPTEST(ClusteringContractExecutor, HandOverSafety) {
    server_id_t alice = generate_uuid(), billy = generate_uuid(),
        carol = generate_uuid();

    /* Bring up the primary, `alice`, with `billy` and `carol` as secondaries */

    executor_tester_context_t context;
    cpu_contract_ids_t cid1 = context.add_contract("*-*",
        quick_contract_simple({alice, billy, carol}, alice));
    context.publish();

    executor_tester_files_t alice_files(alice);
    executor_tester_t alice_exec(&context, &alice_files);
    executor_tester_files_t billy_files(billy);
    executor_tester_t billy_exec(&context, &billy_files);
    executor_tester_files_t carol_files(carol);
    executor_tester_t carol_exec(&context, &carol_files);

    cpu_branch_ids_t branch1;
    alice_exec.check_acks(cid1, contract_ack_t::state_t::primary_need_branch,
        &context.state.branch_history, &branch1);
    billy_exec.check_acks(cid1, contract_ack_t::state_t::secondary_need_primary);
    carol_exec.check_acks(cid1, contract_ack_t::state_t::secondary_need_primary);

    context.remove_contract(cid1);
    cpu_contract_ids_t cid2 = context.add_contract("*-*",
        quick_contract_simple({alice, billy, carol}, alice));
    context.set_current_branches(branch1);
    context.publish();

    alice_exec.check_acks(cid2, contract_ack_t::state_t::primary_ready);
    billy_exec.check_acks(cid2, contract_ack_t::state_t::secondary_streaming);
    carol_exec.check_acks(cid2, contract_ack_t::state_t::secondary_streaming);

    /* Here's the interesting part. We want to hand over control from `alice` to `billy`,
    and we want to make sure that every write that is acked to the client during this
    period is also written to `billy`. */

    /* First start a stream of writes to `alice` */
    write_generator_t write_generator(&alice_exec);
    write_generator.check_writes_work();

    /* Next, tell `alice` to hand the primary over to `billy` */
    context.remove_contract(cid2);
    cpu_contract_ids_t cid3 = context.add_contract("*-*",
        quick_contract_hand_over({alice, billy, carol}, alice, billy));
    context.set_current_branches(branch1);
    context.publish();

    /* Wait until `alice` reports `primary_ready` */
    alice_exec.check_acks(cid3, contract_ack_t::state_t::primary_ready);

    /* Make sure all the writes that `alice` acked have been propagated to `billy` */
    write_generator.verify(&billy_exec);

    /* `alice` should reject further writes */
    try {
        alice_exec.write_primary("hello", "world");
        ADD_FAILURE() << "write_primary() should fail during hand over";
    } catch (const cannot_perform_query_exc_t &exc) {
        EXPECT_EQ("The primary replica is currently changing from one replica to "
            "another. The write was not performed. This error should go away in a "
            "couple of seconds.", std::string(exc.what()));
    }
}

} /* namespace unittest */

