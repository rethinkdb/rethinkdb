// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "arch/io/disk.hpp"
#include "clustering/administration/persist/file.hpp"
#include "clustering/administration/persist/file_keys.hpp"
#include "clustering/administration/persist/raft_storage_interface.hpp"
#include "clustering/administration/tables/split_points.hpp"
#include "clustering/table_contract/contract_metadata.hpp"
#include "unittest/clustering_utils_raft.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

table_config_and_shards_t make_table_config_and_shards() {
    table_config_and_shards_t table_config_and_shards;

    table_config_and_shards.config.basic.name = name_string_t::guarantee_valid("test");
    table_config_and_shards.config.basic.database = generate_uuid();
    table_config_and_shards.config.basic.primary_key = "id";
    table_config_t::shard_t shard;
    shard.primary_replica = server_id_t::generate_server_id();
    shard.all_replicas.insert(shard.primary_replica);
    table_config_and_shards.config.shards.push_back(shard);
    calculate_split_points_for_uuids(1, &table_config_and_shards.shard_scheme);
    table_config_and_shards.config.write_ack_config = write_ack_config_t::MAJORITY;
    table_config_and_shards.config.durability = write_durability_t::HARD;
    table_config_and_shards.server_names.names[shard.primary_replica] =
        std::make_pair(0ul, name_string_t::guarantee_valid("primary"));

    return table_config_and_shards;
}

raft_persistent_state_t<table_raft_state_t> raft_persistent_state_from_metadata_file(
        const temp_directory_t &temp_dir,
        const namespace_id_t &table_id) {
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    cond_t non_interruptor;

    metadata_file_t metadata_file(
        &io_backender,
        temp_dir.path(),
        &get_global_perfmon_collection(),
        &non_interruptor);
    metadata_file_t::read_txn_t read_txn(&metadata_file, &non_interruptor);

    return *table_raft_storage_interface_t(
        &metadata_file, &read_txn, table_id, &non_interruptor).get();
}

TPTEST(ClusteringRaft, StorageRoundtrip) {
    temp_directory_t temp_dir;
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    cond_t non_interruptor;
    namespace_id_t table_id = generate_uuid();

    table_raft_state_t table_raft_state =
        make_new_table_raft_state(make_table_config_and_shards());
    raft_member_id_t raft_member_id(generate_uuid());
    raft_config_t raft_config;
    raft_config.voting_members.insert(raft_member_id);
    raft_persistent_state_t<table_raft_state_t> raft_persistent_state =
        raft_persistent_state_t<table_raft_state_t>::make_initial(
            table_raft_state, raft_config);

    {
        metadata_file_t metadata_file(
            &io_backender,
            temp_dir.path(),
            &get_global_perfmon_collection(),
            [&](metadata_file_t::write_txn_t *, signal_t *) { },
            &non_interruptor);
        metadata_file_t::write_txn_t write_txn(&metadata_file, &non_interruptor);
        table_raft_storage_interface_t table_raft_storage_interface(
            &metadata_file,
            &write_txn,
            table_id,
            raft_persistent_state);
    }

    EXPECT_EQ(
        raft_persistent_state_from_metadata_file(temp_dir, table_id),
        raft_persistent_state);
}

TPTEST(ClusteringRaft, StorageErase) {
    temp_directory_t temp_dir;
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    cond_t non_interruptor;
    namespace_id_t table_id = generate_uuid();

    table_raft_state_t table_raft_state =
        make_new_table_raft_state(make_table_config_and_shards());
    raft_member_id_t raft_member_id(generate_uuid());
    raft_config_t raft_config;
    raft_config.voting_members.insert(raft_member_id);
    raft_persistent_state_t<table_raft_state_t> raft_persistent_state =
        raft_persistent_state_t<table_raft_state_t>::make_initial(
            table_raft_state, raft_config);

    {
        metadata_file_t metadata_file(
            &io_backender,
            temp_dir.path(),
            &get_global_perfmon_collection(),
            [&](metadata_file_t::write_txn_t *, signal_t *) { },
            &non_interruptor);
        metadata_file_t::write_txn_t write_txn(&metadata_file, &non_interruptor);
        table_raft_storage_interface_t table_raft_storage_interface(
            &metadata_file,
            &write_txn,
            table_id,
            raft_persistent_state);

        table_raft_storage_interface.erase(&write_txn, table_id);
    }

    {
        metadata_file_t metadata_file(
            &io_backender,
            temp_dir.path(),
            &get_global_perfmon_collection(),
            &non_interruptor);
        metadata_file_t::read_txn_t read_txn(&metadata_file, &non_interruptor);

        table_raft_stored_header_t table_raft_stored_header;
        EXPECT_FALSE(read_txn.read_maybe(
            mdprefix_table_raft_header().suffix(uuid_to_str(table_id)),
            &table_raft_stored_header,
            &non_interruptor));
    }

}

TPTEST(ClusteringRaft, StorageWriteCurrentTermAndVotedFor) {
    temp_directory_t temp_dir;
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    cond_t non_interruptor;
    namespace_id_t table_id = generate_uuid();

    table_raft_state_t table_raft_state =
        make_new_table_raft_state(make_table_config_and_shards());
    raft_member_id_t raft_member_id(generate_uuid());
    raft_member_id_t raft_member_id_voted_for(generate_uuid());
    raft_config_t raft_config;
    raft_config.voting_members.insert(raft_member_id);
    raft_config.voting_members.insert(raft_member_id_voted_for);
    raft_persistent_state_t<table_raft_state_t> raft_persistent_state =
        raft_persistent_state_t<table_raft_state_t>::make_initial(
            table_raft_state, raft_config);

    {
        scoped_ptr_t<table_raft_storage_interface_t> table_raft_storage_interface;
        metadata_file_t metadata_file(
            &io_backender,
            temp_dir.path(),
            &get_global_perfmon_collection(),
            [&](metadata_file_t::write_txn_t *, signal_t *) { },
            &non_interruptor);
        {
            metadata_file_t::write_txn_t write_txn(&metadata_file, &non_interruptor);
            table_raft_storage_interface.init(new table_raft_storage_interface_t(
                &metadata_file,
                &write_txn,
                table_id,
                raft_persistent_state));
        }

        table_raft_storage_interface->write_current_term_and_voted_for(
            1, raft_member_id_voted_for);
    }

    raft_persistent_state.current_term = 1;
    raft_persistent_state.voted_for = raft_member_id_voted_for;

    EXPECT_EQ(
        raft_persistent_state_from_metadata_file(temp_dir, table_id),
        raft_persistent_state);
}

TPTEST(ClusteringRaft, StorageWriteCommitIndex) {
    temp_directory_t temp_dir;
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    cond_t non_interruptor;
    namespace_id_t table_id = generate_uuid();

    table_raft_state_t table_raft_state =
        make_new_table_raft_state(make_table_config_and_shards());
    raft_member_id_t raft_member_id(generate_uuid());
    raft_config_t raft_config;
    raft_config.voting_members.insert(raft_member_id);
    raft_persistent_state_t<table_raft_state_t> raft_persistent_state =
        raft_persistent_state_t<table_raft_state_t>::make_initial(
            table_raft_state, raft_config);

    {
        scoped_ptr_t<table_raft_storage_interface_t> table_raft_storage_interface;
        metadata_file_t metadata_file(
            &io_backender,
            temp_dir.path(),
            &get_global_perfmon_collection(),
            [&](metadata_file_t::write_txn_t *, signal_t *) { },
            &non_interruptor);
        {
            metadata_file_t::write_txn_t write_txn(&metadata_file, &non_interruptor);
            table_raft_storage_interface.init(new table_raft_storage_interface_t(
                &metadata_file,
                &write_txn,
                table_id,
                raft_persistent_state));
        }

        table_raft_storage_interface->write_commit_index(1);
    }

    raft_persistent_state.commit_index = 1;

    EXPECT_EQ(
        raft_persistent_state_from_metadata_file(temp_dir, table_id),
        raft_persistent_state);
}

TPTEST(ClusteringRaft, StorageWriteLogReplaceTail) {
    temp_directory_t temp_dir;
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    cond_t non_interruptor;
    namespace_id_t table_id = generate_uuid();

    table_raft_state_t table_raft_state =
        make_new_table_raft_state(make_table_config_and_shards());
    raft_member_id_t raft_member_id(generate_uuid());
    raft_config_t raft_config;
    raft_config.voting_members.insert(raft_member_id);
    raft_persistent_state_t<table_raft_state_t> raft_persistent_state =
        raft_persistent_state_t<table_raft_state_t>::make_initial(
            table_raft_state, raft_config);

    raft_log_entry_t<table_raft_state_t> raft_log_entry;
    raft_log_entry.type = raft_log_entry_type_t::regular;
    raft_log_entry.term = 1;
    table_raft_state_t::change_t::set_table_config_t set_table_config;
    set_table_config.new_config = make_table_config_and_shards();
    raft_log_entry.change = set_table_config;

    raft_log_t<table_raft_state_t> raft_log;
    raft_log.prev_index = 0;
    raft_log.prev_term = 0;
    raft_log.append(raft_log_entry);

    {
        scoped_ptr_t<table_raft_storage_interface_t> table_raft_storage_interface;
        metadata_file_t metadata_file(
            &io_backender,
            temp_dir.path(),
            &get_global_perfmon_collection(),
            [&](metadata_file_t::write_txn_t *, signal_t *) { },
            &non_interruptor);
        {
            metadata_file_t::write_txn_t write_txn(&metadata_file, &non_interruptor);
            table_raft_storage_interface.init(new table_raft_storage_interface_t(
                &metadata_file,
                &write_txn,
                table_id,
                raft_persistent_state));
        }

        table_raft_storage_interface->write_log_replace_tail(raft_log, 1);
    }

    raft_persistent_state.log = raft_log;

    EXPECT_EQ(
        raft_persistent_state_from_metadata_file(temp_dir, table_id),
        raft_persistent_state);
}

TPTEST(ClusteringRaft, StorageWriteLogAppendOne) {
    temp_directory_t temp_dir;
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    cond_t non_interruptor;
    namespace_id_t table_id = generate_uuid();

    table_raft_state_t table_raft_state =
        make_new_table_raft_state(make_table_config_and_shards());
    raft_member_id_t raft_member_id(generate_uuid());
    raft_config_t raft_config;
    raft_config.voting_members.insert(raft_member_id);
    raft_persistent_state_t<table_raft_state_t> raft_persistent_state =
        raft_persistent_state_t<table_raft_state_t>::make_initial(
            table_raft_state, raft_config);

    raft_log_entry_t<table_raft_state_t> raft_log_entry;
    raft_log_entry.type = raft_log_entry_type_t::regular;
    raft_log_entry.term = 1;
    table_raft_state_t::change_t::set_table_config_t set_table_config;
    set_table_config.new_config = make_table_config_and_shards();
    raft_log_entry.change = set_table_config;

    {
        scoped_ptr_t<table_raft_storage_interface_t> table_raft_storage_interface;
        metadata_file_t metadata_file(
            &io_backender,
            temp_dir.path(),
            &get_global_perfmon_collection(),
            [&](metadata_file_t::write_txn_t *, signal_t *) { },
            &non_interruptor);
        {
            metadata_file_t::write_txn_t write_txn(&metadata_file, &non_interruptor);
            table_raft_storage_interface.init(new table_raft_storage_interface_t(
                &metadata_file,
                &write_txn,
                table_id,
                raft_persistent_state));
        }

        table_raft_storage_interface->write_log_append_one(raft_log_entry);
    }

    raft_persistent_state.log.append(raft_log_entry);

    EXPECT_EQ(
        raft_persistent_state_from_metadata_file(temp_dir, table_id),
        raft_persistent_state);
}

TPTEST(ClusteringRaft, StorageWriteSnapshot) {
    temp_directory_t temp_dir;
    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    cond_t non_interruptor;
    namespace_id_t table_id = generate_uuid();

    table_raft_state_t table_raft_state =
        make_new_table_raft_state(make_table_config_and_shards());
    raft_member_id_t raft_member_id(generate_uuid());
    raft_config_t raft_config;
    raft_config.voting_members.insert(raft_member_id);
    raft_persistent_state_t<table_raft_state_t> raft_persistent_state =
        raft_persistent_state_t<table_raft_state_t>::make_initial(
            table_raft_state, raft_config);

    raft_complex_config_t raft_complex_config;
    raft_complex_config.config = raft_config;

    {
        scoped_ptr_t<table_raft_storage_interface_t> table_raft_storage_interface;
        metadata_file_t metadata_file(
            &io_backender,
            temp_dir.path(),
            &get_global_perfmon_collection(),
            [&](metadata_file_t::write_txn_t *, signal_t *) { },
            &non_interruptor);
        {
            metadata_file_t::write_txn_t write_txn(&metadata_file, &non_interruptor);
            table_raft_storage_interface.init(new table_raft_storage_interface_t(
                &metadata_file,
                &write_txn,
                table_id,
                raft_persistent_state));
        }

        table_raft_storage_interface->write_snapshot(
            table_raft_state, raft_complex_config, false, 1, 1, 1);
    }

    raft_persistent_state.commit_index = 1;
    raft_persistent_state.snapshot_state = table_raft_state;
    raft_persistent_state.snapshot_config = raft_complex_config;
    raft_persistent_state.log.delete_entries_to(1, 1);

    EXPECT_EQ(
        raft_persistent_state_from_metadata_file(temp_dir, table_id),
        raft_persistent_state);
}

}   /* namespace unittest */

