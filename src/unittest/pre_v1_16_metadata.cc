// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <string>

#include "unittest/gtest.hpp"

#include "clustering/administration/pre_v1_16_metadata.hpp"
#include "clustering/administration/tables/split_points.hpp"

/* This is defined in `clustering/administration/pre_v1_16_metadata.cc` but not exposed
publicly. It's explicitly instantiated for `std::string` where it's defined. */
template<class T>
versioned_t<T> migrate_vclock(
        const pre_v1_16::vclock_t<T> &vclock);

namespace unittest {

template<class T>
pre_v1_16::vclock_t<T> make_vclock(const T &value) {
    pre_v1_16::vclock_t<T> vclock;
    pre_v1_16::version_map_t vmap;
    vmap[generate_uuid()] = 1;
    vclock.values[vmap] = value;
    return vclock;
}

template<class T>
deletable_t<T> make_deletable(const T &value) {
    return deletable_t<T>(value);
}

pre_v1_16::persistable_blueprint_t make_blueprint(
        const pre_v1_16::machines_semilattice_metadata_t &servers,
        const std::vector<std::vector<server_id_t> > &shards) {
    table_shard_scheme_t shard_scheme;
    calculate_split_points_for_uuids(shards.size(), &shard_scheme);

    pre_v1_16::persistable_blueprint_t blueprint;
    for (const auto &pair : servers.machines) {
        if (pair.second.is_deleted()) {
            continue;
        }
        server_id_t server = pair.first;
        pre_v1_16::persistable_blueprint_t::region_to_role_map_t server_roles;
        for (size_t i = 0; i < shards.size(); ++i) {
            blueprint_role_t role;
            if (server == shards[i].front()) {
                role = blueprint_role_primary;
            } else if (std::find(shards[i].begin(), shards[i].end(), server) !=
                    shards[i].end()) {
                role = blueprint_role_secondary;
            } else {
                role = blueprint_role_nothing;
            }
            server_roles[hash_region_t<key_range_t>(shard_scheme.get_shard_range(i))] =
                role;
        }
        blueprint.machines_roles[server] = server_roles;
    }
    return blueprint;
}

void check_blueprint(
        const table_replication_info_t &repli_info,
        const std::vector<std::vector<server_id_t> > &shards) {
    table_shard_scheme_t shard_scheme;
    calculate_split_points_for_uuids(shards.size(), &shard_scheme);
    guarantee(shard_scheme == repli_info.shard_scheme);

    guarantee(shards.size() == repli_info.config.shards.size());
    for (size_t i = 0; i < shards.size(); ++i) {
        const table_config_t::shard_t &actual_shard = repli_info.config.shards[i];
        const std::vector<server_id_t> &expect_shard = shards[i];
        guarantee(actual_shard.primary_replica == expect_shard[0]);
        guarantee(actual_shard.replicas.size() == expect_shard.size());
        for (const server_id_t &server : expect_shard) {
            guarantee(actual_shard.replicas.count(server) == 1);
        }
    }
}

TEST(PreV116Metadata, Migrate) {
    pre_v1_16::cluster_semilattice_metadata_t pre_cluster;

    pre_v1_16::datacenter_id_t datacenter1_id = generate_uuid();
    pre_v1_16::datacenter_semilattice_metadata_t pre_datacenter1;
    pre_datacenter1.name = make_vclock(name_string_t::guarantee_valid("datacenter1"));
    pre_cluster.datacenters.datacenters[datacenter1_id] =
        make_deletable(pre_datacenter1);

    pre_v1_16::datacenter_id_t datacenter2_id = generate_uuid();
    pre_v1_16::datacenter_semilattice_metadata_t pre_datacenter2;
    pre_datacenter2.name = make_vclock(name_string_t::guarantee_valid("datacenter2"));
    pre_cluster.datacenters.datacenters[datacenter2_id] =
        make_deletable(pre_datacenter2);

    pre_v1_16::datacenter_id_t deleted_datacenter_id = generate_uuid();
    pre_cluster.datacenters.datacenters[deleted_datacenter_id].mark_deleted();

    server_id_t server1_id = generate_uuid();
    pre_v1_16::machine_semilattice_metadata_t pre_server1;
    pre_server1.name = make_vclock(name_string_t::guarantee_valid("server1"));
    pre_server1.datacenter = make_vclock(datacenter1_id);
    pre_cluster.machines.machines[server1_id] = make_deletable(pre_server1);

    server_id_t server2_id = generate_uuid();
    pre_v1_16::machine_semilattice_metadata_t pre_server2;
    pre_server2.name = make_vclock(name_string_t::guarantee_valid("server2"));
    pre_server2.datacenter = make_vclock(datacenter2_id);
    pre_cluster.machines.machines[server2_id] = make_deletable(pre_server2);

    server_id_t server3_id = generate_uuid();
    pre_v1_16::machine_semilattice_metadata_t pre_server3;
    pre_server3.name = make_vclock(name_string_t::guarantee_valid("server3"));
    pre_server3.datacenter = make_vclock(nil_uuid());
    pre_cluster.machines.machines[server3_id] = make_deletable(pre_server3);

    server_id_t deleted_server_id = generate_uuid();
    pre_cluster.machines.machines[deleted_server_id].mark_deleted();

    database_id_t database_id = generate_uuid();
    pre_v1_16::database_semilattice_metadata_t pre_database;
    pre_database.name = make_vclock(name_string_t::guarantee_valid("database"));
    pre_cluster.databases.databases[database_id] = make_deletable(pre_database);

    database_id_t deleted_database_id = generate_uuid();
    pre_cluster.databases.databases[deleted_database_id].mark_deleted();

    pre_v1_16::namespaces_semilattice_metadata_t pre_namespaces;

    namespace_id_t table1_id = generate_uuid();
    pre_v1_16::namespace_semilattice_metadata_t pre_table1;
    pre_table1.blueprint = make_vclock(make_blueprint(pre_cluster.machines, {
        { server1_id },
        { server1_id }
        }));
    pre_table1.primary_datacenter = make_vclock(nil_uuid());
    pre_table1.replica_affinities = make_vclock(
        std::map<pre_v1_16::datacenter_id_t, int32_t>({
            { nil_uuid(), 1 }
        }));
    pre_table1.ack_expectations = make_vclock(
        std::map<pre_v1_16::datacenter_id_t, pre_v1_16::ack_expectation_t>({
            { nil_uuid(), pre_v1_16::ack_expectation_t({ 1, true }) }
        }));
    pre_table1.name = make_vclock(name_string_t::guarantee_valid("table1"));
    pre_table1.primary_key = make_vclock(std::string("table1_pkey"));
    pre_table1.database = make_vclock(database_id);
    pre_namespaces.namespaces[table1_id] = make_deletable(pre_table1);

    namespace_id_t table2_id = generate_uuid();
    pre_v1_16::namespace_semilattice_metadata_t pre_table2;
    pre_table2.blueprint = make_vclock(make_blueprint(pre_cluster.machines, {
        { server1_id, server2_id }
        }));
    pre_table2.primary_datacenter = make_vclock(datacenter1_id);
    pre_table2.replica_affinities = make_vclock(
        std::map<pre_v1_16::datacenter_id_t, int32_t>({
            { datacenter1_id, 1 },
            { datacenter2_id, 1 }
        }));
    pre_table2.ack_expectations = make_vclock(
        std::map<pre_v1_16::datacenter_id_t, pre_v1_16::ack_expectation_t>({
            { datacenter1_id, pre_v1_16::ack_expectation_t({ 1, false }) }
        }));
    pre_table2.name = make_vclock(name_string_t::guarantee_valid("table2"));
    pre_table2.primary_key = make_vclock(std::string("table2_pkey"));
    pre_table2.database = make_vclock(database_id);
    pre_namespaces.namespaces[table2_id] = make_deletable(pre_table2);

    namespace_id_t table3_id = generate_uuid();
    pre_v1_16::namespace_semilattice_metadata_t pre_table3;
    pre_table3.blueprint = make_vclock(make_blueprint(pre_cluster.machines, {
        { server1_id, server2_id },
        { server3_id, server2_id },
        }));
    pre_table3.primary_datacenter = make_vclock(nil_uuid());
    pre_table3.replica_affinities = make_vclock(
        std::map<pre_v1_16::datacenter_id_t, int32_t>({
            { nil_uuid(), 1 },
            { datacenter2_id, 1}
        }));
    pre_table3.ack_expectations = make_vclock(
        std::map<pre_v1_16::datacenter_id_t, pre_v1_16::ack_expectation_t>({
            { datacenter2_id, pre_v1_16::ack_expectation_t({ 1, false }) }
        }));
    pre_table3.name = make_vclock(name_string_t::guarantee_valid("table3"));
    pre_table3.primary_key = make_vclock(std::string("table3_pkey"));
    pre_table3.database = make_vclock(database_id);
    pre_namespaces.namespaces[table3_id] = make_deletable(pre_table3);

    namespace_id_t deleted_table_id = generate_uuid();
    pre_namespaces.namespaces[deleted_table_id].mark_deleted();

    pre_cluster.rdb_namespaces =
        cow_ptr_t<pre_v1_16::namespaces_semilattice_metadata_t>(pre_namespaces);

    cluster_semilattice_metadata_t post_cluster =
        migrate_cluster_metadata_to_v1_16(pre_cluster);

    EXPECT_EQ(4, post_cluster.servers.servers.size());

    {
        auto it = post_cluster.servers.servers.find(server1_id);
        ASSERT_FALSE(it == post_cluster.servers.servers.end());
        ASSERT_FALSE(it->second.is_deleted());
        EXPECT_EQ("server1", it->second.get_ref().name.get_ref().str());
        std::set<name_string_t> tags = it->second.get_ref().tags.get_ref();
        EXPECT_EQ(2, tags.size());
        auto jt = tags.begin();
        EXPECT_EQ("datacenter1", jt->str());
        ++jt;
        EXPECT_EQ("default", jt->str());
    }

    {
        auto it = post_cluster.servers.servers.find(server2_id);
        ASSERT_FALSE(it == post_cluster.servers.servers.end());
        ASSERT_FALSE(it->second.is_deleted());
        EXPECT_EQ(it->second.get_ref().name.get_ref().str(), "server2");
        std::set<name_string_t> tags = it->second.get_ref().tags.get_ref();
        EXPECT_EQ(2, tags.size());
        auto jt = tags.begin();
        EXPECT_EQ("datacenter2", jt->str());
        ++jt;
        EXPECT_EQ("default", jt->str());
    }

    {
        auto it = post_cluster.servers.servers.find(server3_id);
        ASSERT_FALSE(it == post_cluster.servers.servers.end());
        ASSERT_FALSE(it->second.is_deleted());
        EXPECT_EQ(it->second.get_ref().name.get_ref().str(), "server3");
        std::set<name_string_t> tags = it->second.get_ref().tags.get_ref();
        EXPECT_EQ(1, tags.size());
        auto jt = tags.begin();
        EXPECT_EQ("default", jt->str());
    }

    {
        auto it = post_cluster.servers.servers.find(deleted_server_id);
        ASSERT_FALSE(it == post_cluster.servers.servers.end());
        ASSERT_TRUE(it->second.is_deleted());
    }

    EXPECT_EQ(2, post_cluster.databases.databases.size());

    {
        auto it = post_cluster.databases.databases.find(database_id);
        ASSERT_FALSE(it == post_cluster.databases.databases.end());
        ASSERT_FALSE(it->second.is_deleted());
        EXPECT_EQ("database", it->second.get_ref().name.get_ref().str());
    }

    {
        auto it = post_cluster.databases.databases.find(deleted_database_id);
        ASSERT_FALSE(it == post_cluster.databases.databases.end());
        ASSERT_TRUE(it->second.is_deleted());
    }

    EXPECT_EQ(4, post_cluster.rdb_namespaces->namespaces.size());

    {
        auto it = post_cluster.rdb_namespaces->namespaces.find(table1_id);
        ASSERT_FALSE(it == post_cluster.rdb_namespaces->namespaces.end());
        ASSERT_FALSE(it->second.is_deleted());
        EXPECT_EQ("table1", it->second.get_ref().name.get_ref().str());
        EXPECT_EQ(database_id, it->second.get_ref().database.get_ref());
        EXPECT_EQ("table1_pkey", it->second.get_ref().primary_key.get_ref());
        table_replication_info_t post_repli_info =
            it->second.get_ref().replication_info.get_ref();
        check_blueprint(post_repli_info, {
            { server1_id },
            { server1_id }
            });
        EXPECT_TRUE(post_repli_info.config.write_ack_config.mode ==
            write_ack_config_t::mode_t::single);
        EXPECT_EQ(write_durability_t::HARD, post_repli_info.config.durability);
    }

    {
        auto it = post_cluster.rdb_namespaces->namespaces.find(table2_id);
        ASSERT_FALSE(it == post_cluster.rdb_namespaces->namespaces.end());
        ASSERT_FALSE(it->second.is_deleted());
        EXPECT_EQ("table2", it->second.get_ref().name.get_ref().str());
        EXPECT_EQ(database_id, it->second.get_ref().database.get_ref());
        EXPECT_EQ("table2_pkey", it->second.get_ref().primary_key.get_ref());
        table_replication_info_t post_repli_info =
            it->second.get_ref().replication_info.get_ref();
        check_blueprint(post_repli_info, {
            { server1_id, server2_id }
            });
        EXPECT_TRUE(post_repli_info.config.write_ack_config.mode ==
            write_ack_config_t::mode_t::single);
        EXPECT_EQ(write_durability_t::SOFT, post_repli_info.config.durability);
    }

    {
        auto it = post_cluster.rdb_namespaces->namespaces.find(table3_id);
        ASSERT_FALSE(it == post_cluster.rdb_namespaces->namespaces.end());
        ASSERT_FALSE(it->second.is_deleted());
        EXPECT_EQ("table3", it->second.get_ref().name.get_ref().str());
        EXPECT_EQ(database_id, it->second.get_ref().database.get_ref());
        EXPECT_EQ("table3_pkey", it->second.get_ref().primary_key.get_ref());
        table_replication_info_t post_repli_info =
            it->second.get_ref().replication_info.get_ref();
        check_blueprint(post_repli_info, {
            { server1_id, server2_id },
            { server3_id, server2_id },
            });
        ASSERT_TRUE(post_repli_info.config.write_ack_config.mode ==
            write_ack_config_t::mode_t::complex);
        ASSERT_EQ(1, post_repli_info.config.write_ack_config.complex_reqs.size());
        write_ack_config_t::req_t post_req =
            post_repli_info.config.write_ack_config.complex_reqs[0];
        ASSERT_EQ(1, post_req.replicas.size());
        EXPECT_EQ(server2_id, *post_req.replicas.begin());
        EXPECT_TRUE(post_req.mode == write_ack_config_t::mode_t::single);
        EXPECT_EQ(write_durability_t::SOFT, post_repli_info.config.durability);
    }

    {
        auto it = post_cluster.rdb_namespaces->namespaces.find(deleted_table_id);
        ASSERT_FALSE(it == post_cluster.rdb_namespaces->namespaces.end());
        ASSERT_TRUE(it->second.is_deleted());
    }
}

TEST(PreV116Metadata, VectorClock) {
    /* Make sure that if we migrate two copies of the same vector clock, and one is more
    up-to-date than the other, then the more up-to-date vector clock will produce a more
    up-to-date `versioned_t`. */
    pre_v1_16::vclock_t<std::string> vclock1, vclock2;
    pre_v1_16::version_map_t vmap;
    vmap[generate_uuid()] = 1;
    vclock1.values[vmap] = "foo";
    vmap[generate_uuid()] = 1;
    vclock2.values[vmap] = "bar";
    versioned_t<std::string> vers1 = migrate_vclock(vclock1);
    versioned_t<std::string> vers2 = migrate_vclock(vclock2);
    versioned_t<std::string> join1 = vers1, join2 = vers2;
    semilattice_join(&join1, vers2);
    semilattice_join(&join2, vers1);
    EXPECT_EQ("bar", join1.get_ref());
    EXPECT_EQ("bar", join2.get_ref());
}

}  // namespace unittest
