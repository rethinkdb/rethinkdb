// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist/migrate/migrate_v1_16.hpp"

#include "buffer_cache/alt.hpp"
#include "buffer_cache/blob.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/persist/migrate/migrate_v1_14.hpp"
#include "clustering/administration/persist/raft_storage_interface.hpp"
#include "clustering/administration/persist/file_keys.hpp"
#include "clustering/immediate_consistency/history.hpp"
#include "containers/binary_blob.hpp"
#include "rdb_protocol/store.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/merger.hpp"
#include "serializer/translator.hpp"

struct cluster_metadata_superblock_t {
    block_magic_t magic;

    server_id_t server_id;

    static const int METADATA_BLOB_MAXREFLEN = 1500;
    char metadata_blob[METADATA_BLOB_MAXREFLEN];

    static const int BRANCH_HISTORY_BLOB_MAXREFLEN = 500;
    char rdb_branch_history_blob[BRANCH_HISTORY_BLOB_MAXREFLEN];
};

struct auth_metadata_superblock_t {
    block_magic_t magic;

    static const int METADATA_BLOB_MAXREFLEN = 1500;
    char metadata_blob[METADATA_BLOB_MAXREFLEN];
};

template <cluster_version_t>
struct cluster_metadata_magic_t {
    static const block_magic_t value;
};

template <>
const block_magic_t
    cluster_metadata_magic_t<cluster_version_t::v1_14>::value
        = { { 'R', 'D', 'm', 'e' } };
template <>
const block_magic_t
    cluster_metadata_magic_t<cluster_version_t::v1_15>::value
        = { { 'R', 'D', 'm', 'f' } };
template <>
const block_magic_t
    cluster_metadata_magic_t<cluster_version_t::v1_16>::value
        = { { 'R', 'D', 'm', 'g' } };
template <>
const block_magic_t
    cluster_metadata_magic_t<cluster_version_t::v2_0>::value
        = { { 'R', 'D', 'm', 'h' } };

cluster_version_t cluster_superblock_version(const cluster_metadata_superblock_t *sb) {
    if (sb->magic == cluster_metadata_magic_t<cluster_version_t::v1_14>::value) {
        return cluster_version_t::v1_14;
    } else if (sb->magic == cluster_metadata_magic_t<cluster_version_t::v1_15>::value) {
        return cluster_version_t::v1_15;
    } else if (sb->magic == cluster_metadata_magic_t<cluster_version_t::v1_16>::value) {
        return cluster_version_t::v1_16;
    } else if (sb->magic == cluster_metadata_magic_t<cluster_version_t::v2_0>::value) {
        return cluster_version_t::v2_0;
    } else {
        fail_due_to_user_error("Migration of cluster metadata could not be performed, it "
                               "is only supported for metadata from v1.14.x and later.");
    }
}

template <cluster_version_t>
struct auth_metadata_magic_t {
    static const block_magic_t value;
};

template <>
const block_magic_t auth_metadata_magic_t<cluster_version_t::v1_14>::value =
    { { 'R', 'D', 'm', 'e' } };

template <>
const block_magic_t auth_metadata_magic_t<cluster_version_t::v1_15>::value =
    { { 'R', 'D', 'm', 'f' } };

template <>
const block_magic_t auth_metadata_magic_t<cluster_version_t::v1_16>::value =
    { { 'R', 'D', 'm', 'g' } };

template <>
const block_magic_t auth_metadata_magic_t<cluster_version_t::v2_0>::value =
    { { 'R', 'D', 'm', 'h' } };

cluster_version_t auth_superblock_version(const auth_metadata_superblock_t *sb) {
    if (sb->magic == auth_metadata_magic_t<cluster_version_t::v1_14>::value) {
        return cluster_version_t::v1_14;
    } else if (sb->magic == auth_metadata_magic_t<cluster_version_t::v1_15>::value) {
        return cluster_version_t::v1_15;
    } else if (sb->magic == auth_metadata_magic_t<cluster_version_t::v1_16>::value) {
        return cluster_version_t::v1_16;
    } else if (sb->magic == auth_metadata_magic_t<cluster_version_t::v2_0>::value) {
        return cluster_version_t::v2_0;
    } else {
        fail_due_to_user_error("Migration of auth metadata could not be performed, it "
                               "is only supported for metadata from v1.14.x and later.");
    }
}

struct regioned_version_t {
    regioned_version_t(const region_t &r, const ::version_t &v) :
        region(r), version(v) { }
    region_t region;
    ::version_t version;
};

bool operator <(const regioned_version_t &a, const regioned_version_t &b) {
    if (a.version.branch == b.version.branch) {
        if (a.version.timestamp == b.version.timestamp) {
            return a.region < b.region;
        }
        return a.version.timestamp < b.version.timestamp;
    }
    return a.version.branch < b.version.branch;
}

static void read_blob(buf_parent_t parent, const char *ref, int maxreflen,
                      const std::function<archive_result_t(read_stream_t *)> &reader) {
    blob_t blob(parent.cache()->max_block_size(),
                const_cast<char *>(ref), maxreflen);
    blob_acq_t acq_group;
    buffer_group_t group;
    blob.expose_all(parent, access_t::read, &group, &acq_group);
    buffer_group_read_stream_t ss(const_view(&group));
    archive_result_t res = reader(&ss);
    guarantee_deserialization(res, "T (template code)");
}

void migrate_heartbeat(metadata_file_t::write_txn_t *out,
                       signal_t *interruptor) {
    out->write(mdkey_heartbeat_semilattices(),
               heartbeat_semilattice_metadata_t(), interruptor);
}

void migrate_server(const server_id_t &server_id,
                    const metadata_v1_16::cluster_semilattice_metadata_t &metadata,
                    metadata_file_t::write_txn_t *out,
                    signal_t *interruptor) {
    auto self_it = metadata.servers.servers.find(server_id);
    guarantee(self_it != metadata.servers.servers.end(),
              "Migration of cluster metadata failed, could not find own server config.");
    if (self_it->second.is_deleted()) {
        fail_due_to_user_error("Migration of cluster metadata failed because "
                               "this server was already deleted from the cluster.");
    }

    server_config_versioned_t new_config;
    new_config.version = 1;
    new_config.config.name = self_it->second.get_ref().name.get_ref();
    new_config.config.tags = self_it->second.get_ref().tags.get_ref();
    new_config.config.cache_size_bytes =
        self_it->second.get_ref().cache_size_bytes.get_ref();

    out->write(mdkey_server_config(), new_config, interruptor);
    out->write(mdkey_server_id(), server_id, interruptor);
}

void migrate_databases(const metadata_v1_16::cluster_semilattice_metadata_t &metadata,
                       metadata_file_t::write_txn_t *out,
                       signal_t *interruptor) {
    ::cluster_semilattice_metadata_t new_metadata;
    for (auto const &pair : metadata.databases.databases) {
        if (!pair.second.is_deleted()) {
            ::database_semilattice_metadata_t db;
            db.name = pair.second.get_ref().name;
            auto res = new_metadata.databases.databases.insert(
                std::make_pair(pair.first, make_deletable(db)));
            guarantee(res.second);
        } else {
            auto res = new_metadata.databases.databases.insert(
                std::make_pair(pair.first,
                               deletable_t<database_semilattice_metadata_t>()));
            guarantee(res.second);
            res.first->second.mark_deleted();
        }
    }
    out->write(mdkey_cluster_semilattices(), new_metadata, interruptor);
}

::branch_history_t migrate_branch_ids(
        const namespace_id_t &table_id,
        const std::vector<regioned_version_t> &table_versions,
        const metadata_v1_16::branch_history_t &branch_history,
        metadata_file_t::write_txn_t *out,
        signal_t *interruptor) {
    branch_history_t result;
    std::set<branch_id_t> seen_branches{ nil_uuid() };
    std::deque<branch_id_t> branches_to_save;

    for (auto const &rv : table_versions) {
        auto const branch_it = branch_history.branches.find(rv.version.branch);

        // If we don't find the branch, we assume that it is referencing a dummy branch
        // that we created to replace an incoherent version_range_t.  In this case, we
        // want the dummy branch to inherit directly from version_t::zero().  This will
        // leave the data as-is until such time that a raft cluster is established for
        // the table and a more-recent coherent replica is found.  At that point the
        // incoherent data shall be erased and backfilled from the coherent replica.
        if (branch_it == branch_history.branches.end()) {
            branch_birth_certificate_t new_birth_certificate;
            new_birth_certificate.initial_timestamp =
                version_t::zero().timestamp;
            new_birth_certificate.origin =
                region_map_t<version_t>(rv.region, version_t::zero());

            result.branches.insert(std::make_pair(rv.version.branch,
                                                  new_birth_certificate));

            out->write(mdprefix_branch_birth_certificate().suffix(
                           uuid_to_str(table_id) + "/" +
                           uuid_to_str(rv.version.branch)),
                       new_birth_certificate, interruptor);
        } else if (seen_branches.count(rv.version.branch) == 0) {
            seen_branches.insert(rv.version.branch);
            branches_to_save.push_back(rv.version.branch);
        }
    }

    while (!branches_to_save.empty()) {
        auto const branch_it = branch_history.branches.find(branches_to_save.front());
        guarantee(branch_it != branch_history.branches.end());
        branches_to_save.pop_front();

        branch_birth_certificate_t new_birth_certificate;

        new_birth_certificate.initial_timestamp = branch_it->second.initial_timestamp;
        new_birth_certificate.origin =
            branch_it->second.origin.map(branch_it->second.region,
            [&](const metadata_v1_16::version_range_t &v) -> ::version_t {
                ::version_t res(v.earliest.branch, v.earliest.timestamp);
                guarantee(v.is_coherent());
                if (seen_branches.count(res.branch) == 0) {
                    seen_branches.insert(res.branch);
                    branches_to_save.push_back(res.branch);
                }
                return res;
            });

        result.branches.insert(std::make_pair(branch_it->first, new_birth_certificate));

        out->write(mdprefix_branch_birth_certificate().suffix(
                       uuid_to_str(table_id) + "/" + uuid_to_str(branch_it->first)),
                   new_birth_certificate, interruptor);
    }

    return result;
}

// This function will use a new uuid rather than the timestamp's tiebreaker because
// we are combining multiple timestamps into one - we could potentially lose changes across
// the cluster.  Rather than have conflicting data in the committed raft log under the same
// epoch (on two different servers), we may instead lose a configuration change.  This should
// only realistically happen if configuration changes were made while the cluster was in the
// process of shutting down before the upgrade.
template <typename... Args>
multi_table_manager_timestamp_t max_versioned_timestamp(const versioned_t<Args> &...args) {
    std::vector<time_t> times = { args.get_timestamp()... };
    time_t ts = *std::max_element(times.begin(), times.end());

    multi_table_manager_timestamp_t res;
    res.epoch = multi_table_manager_timestamp_t::epoch_t::migrate(ts);
    res.log_index = 0;
    return res;
}

bool is_server_deleted(const server_id_t &server_id,
                       const metadata_v1_16::servers_semilattice_metadata_t &servers_metadata) {
    auto serv_it = servers_metadata.servers.find(server_id);
    return (serv_it == servers_metadata.servers.end()) ||
           serv_it->second.is_deleted();
}

void migrate_table(const server_id_t &this_server_id,
                   const namespace_id_t &table_id,
                   const metadata_v1_16::namespace_semilattice_metadata_t &table_metadata,
                   const metadata_v1_16::servers_semilattice_metadata_t &servers_metadata,
                   const std::map<std::string, std::pair<sindex_config_t, sindex_status_t> > &sindexes,
                   const ::branch_history_t &branch_history,
                   metadata_file_t::write_txn_t *out,
                   signal_t *interruptor) {
    metadata_v1_16::table_replication_info_t old_config = table_metadata.replication_info.get_ref();

    table_config_and_shards_t config;
    config.config.basic.name = table_metadata.name.get_ref();
    config.config.basic.database = table_metadata.database.get_ref();
    config.config.basic.primary_key = table_metadata.primary_key.get_ref();
    config.config.write_ack_config =
        old_config.config.write_ack_config.mode ==
            metadata_v1_16::write_ack_config_t::mode_t::single ?
                ::write_ack_config_t::SINGLE : ::write_ack_config_t::MAJORITY;
    config.config.durability = old_config.config.durability;
    config.shard_scheme.split_points = old_config.shard_scheme.split_points;

    // Scan the servers in the old shard config - need to remove deleted and nil servers
    for (size_t i = 0; i < old_config.config.shards.size(); ++i) {
        metadata_v1_16::table_config_t::shard_t &s = old_config.config.shards[i];
        std::set<server_id_t> replicas_copy = s.replicas;
        for (auto const &serv : replicas_copy) {
            if (is_server_deleted(serv, servers_metadata)) {
                s.replicas.erase(serv);
            }
        }
        if (is_server_deleted(s.primary_replica, servers_metadata)) {
            if (s.replicas.empty()) {
                logWRN("Table %s has no replicas for the region %s\n",
                       uuid_to_str(table_id).c_str(),
                       old_config.shard_scheme.get_shard_range(i).print().c_str());
                s.replicas.insert(s.primary_replica);
            } else {
                s.primary_replica = *s.replicas.begin();
            }
        }
    }

    std::set<server_id_t> used_servers;
    for (auto const &s : old_config.config.shards) {
        ::table_config_t::shard_t new_shard;
        new_shard.all_replicas = s.replicas;
        new_shard.primary_replica = s.primary_replica;
        config.config.shards.push_back(std::move(new_shard));
        used_servers.insert(s.replicas.begin(), s.replicas.end());
    }

    for (auto const &pair : sindexes) {
        config.config.sindexes.insert(std::make_pair(pair.first, pair.second.first));
    }

    for (auto const &serv_id : used_servers) {
        auto serv_it = servers_metadata.servers.find(serv_id);
        if (serv_it != servers_metadata.servers.end() && !serv_it->second.is_deleted()) {
            config.server_names.names.insert(std::make_pair(
                serv_id, std::make_pair(1, serv_it->second.get_ref().name.get_ref())));
        } else {
            config.server_names.names.insert(std::make_pair(
                serv_id,
                std::make_pair(1, name_string_t::guarantee_valid("__deleted_server__"))));
        }
    }

    table_raft_state_t raft_state = make_new_table_raft_state(config);
    raft_state.branch_history = branch_history;
    auto own_membership = raft_state.member_ids.find(this_server_id);

    /* Set the `after_emergency_repair` flag for each contract in the config.
    This is important to maintain certain invariants about the branch history. */
    for (auto &contract : raft_state.contracts) {
        contract.second.second.after_emergency_repair = true;
    }

    if (own_membership == raft_state.member_ids.end()) {
        // This server is not involved in the table - write an inactive state
        table_inactive_persistent_state_t state;
        state.timestamp = max_versioned_timestamp(table_metadata.name,
                                                  table_metadata.database,
                                                  table_metadata.primary_key);
        state.second_hand_config.name = table_metadata.name.get_ref();
        state.second_hand_config.database = table_metadata.database.get_ref();
        state.second_hand_config.primary_key = table_metadata.primary_key.get_ref();

        out->write(mdprefix_table_inactive().suffix(uuid_to_str(table_id)), state, interruptor);
    } else {
        // This server is involved in the table - write an active state
        raft_config_t raft_config;
        for (auto const &pair : raft_state.member_ids) {
            raft_config.voting_members.insert(pair.second);
        }

        raft_persistent_state_t<table_raft_state_t> persistent_state =
            raft_persistent_state_t<table_raft_state_t>::make_initial(raft_state, raft_config);

        table_active_persistent_state_t active_state;
        active_state.epoch = max_versioned_timestamp(table_metadata.name,
                                                     table_metadata.database,
                                                     table_metadata.primary_key,
                                                     table_metadata.replication_info).epoch;
        guarantee(own_membership != raft_state.member_ids.end());
        active_state.raft_member_id = own_membership->second;
        out->write(mdprefix_table_active().suffix(uuid_to_str(table_id)), active_state, interruptor);

        // The `table_raft_storage_interface_t` constructor will persist the header, snapshot, and logs
        table_raft_storage_interface_t storage_interface(
            nullptr, out, table_id, persistent_state);
    }
}

void check_for_obsolete_sindexes(io_backender_t *io_backender,
                                 const base_path_t &base_path,
                                 const metadata_v1_16::cluster_semilattice_metadata_t &metadata,
                                 signal_t *interruptor) {
    dummy_cache_balancer_t balancer(GIGABYTE);
    auto &tables = metadata.rdb_namespaces.namespaces;
    pmap(tables.begin(), tables.end(),
         [&](std::pair<const namespace_id_t,
                       deletable_t<metadata_v1_16::namespace_semilattice_metadata_t> > &info) {
            if (!info.second.is_deleted()) {
                perfmon_collection_t dummy_stats;
                serializer_filepath_t table_path(base_path, uuid_to_str(info.first));
                filepath_file_opener_t file_opener(table_path, io_backender);
                scoped_ptr_t<log_serializer_t> inner_serializer(
                    new log_serializer_t(log_serializer_t::dynamic_config_t(),
                                              &file_opener, &dummy_stats));
                merger_serializer_t merger_serializer(std::move(inner_serializer),
                                                      MERGER_SERIALIZER_MAX_ACTIVE_WRITES);
                std::vector<serializer_t *> underlying({ &merger_serializer });
                serializer_multiplexer_t multiplexer(underlying);

                pmap(CPU_SHARDING_FACTOR, [&](int index) {
                        perfmon_collection_t inner_dummy_stats;
                        store_t store(cpu_sharding_subspace(index),
                                      multiplexer.proxies[index],
                                      &balancer,
                                      "table_migration",
                                      false,
                                      &inner_dummy_stats,
                                      nullptr,
                                      io_backender,
                                      base_path,
                                      info.first,
                                      /* Important: Don't update indexes yet. */
                                      update_sindexes_t::LEAVE_ALONE);

                        store.sindex_list(interruptor);
                    });
            }
        });
}

void migrate_tables(io_backender_t *io_backender,
                    const base_path_t &base_path,
                    const server_id_t &this_server_id,
                    const metadata_v1_16::cluster_semilattice_metadata_t &metadata,
                    const metadata_v1_16::branch_history_t &old_branch_history,
                    metadata_file_t::write_txn_t *out,
                    signal_t *interruptor) {
    dummy_cache_balancer_t balancer(GIGABYTE);
    auto &tables = metadata.rdb_namespaces.namespaces;
    pmap(tables.begin(), tables.end(),
         [&](std::pair<const namespace_id_t,
                       deletable_t<metadata_v1_16::namespace_semilattice_metadata_t> > &info) {
            // We don't need to store anything for deleted tables
            if (!info.second.is_deleted()) {
                perfmon_collection_t dummy_stats;
                serializer_filepath_t table_path(base_path, uuid_to_str(info.first));
                filepath_file_opener_t file_opener(table_path, io_backender);
                scoped_ptr_t<log_serializer_t> inner_serializer(
                    new log_serializer_t(log_serializer_t::dynamic_config_t(),
                                              &file_opener, &dummy_stats));
                merger_serializer_t merger_serializer(std::move(inner_serializer),
                                                      MERGER_SERIALIZER_MAX_ACTIVE_WRITES);
                std::vector<serializer_t *> underlying({ &merger_serializer });
                serializer_multiplexer_t multiplexer(underlying);

                std::vector<regioned_version_t> table_versions;
                std::map<std::string, std::pair<sindex_config_t, sindex_status_t> > sindex_list;

                pmap(CPU_SHARDING_FACTOR, [&](int index) {
                        perfmon_collection_t inner_dummy_stats;
                        store_t store(cpu_sharding_subspace(index),
                                      multiplexer.proxies[index],
                                      &balancer,
                                      "table_migration",
                                      false,
                                      &inner_dummy_stats,
                                      nullptr,
                                      io_backender,
                                      base_path,
                                      info.first,
                                      update_sindexes_t::UPDATE);

                        if (index == 0) {
                            sindex_list = store.sindex_list(interruptor);
                        }

                        read_token_t read_token;
                        if (store.metainfo_version(&read_token,
                                                   interruptor) == cluster_version_t::v2_0) {
                            write_token_t write_token;
                            store.new_write_token(&write_token);
                            store.migrate_metainfo(
                                order_token_t::ignore, &write_token,
                                cluster_version_t::v2_0, cluster_version_t::v2_1,
                                [&](const region_t &r, const binary_blob_t &blob) -> binary_blob_t {
                                    auto const &v =
                                        binary_blob_t::get<metadata_v1_16::version_range_t>(blob);
                                    ::version_t res(v.earliest.branch, v.earliest.timestamp);
                                    if (!v.is_coherent()) {
                                        // Create a dummy branch that will be assigned a birth
                                        // certificate off of version_t::zero
                                        res = ::version_t(generate_uuid(),
                                                          state_timestamp_t::zero());
                                    }
                                    if (!res.branch.is_nil()) {
                                        table_versions.push_back(regioned_version_t(r, res));
                                    }
                                    return binary_blob_t::make<version_t>(res);
                                }, interruptor);
                        }

                        guarantee(store.metainfo_version(&read_token,
                                                         interruptor) == cluster_version_t::v2_1);
                    });

                branch_history_t new_branch_history =
                    migrate_branch_ids(info.first,
                                       table_versions,
                                       old_branch_history,
                                       out,
                                       interruptor);

                migrate_table(this_server_id,
                              info.first,
                              info.second.get_ref(),
                              metadata.servers,
                              sindex_list,
                              new_branch_history,
                              out,
                              interruptor);
            }
         });

}

void migrate_cluster_metadata_to_v2_2(io_backender_t *io_backender,
                                      const base_path_t &base_path,
                                      buf_parent_t buf_parent,
                                      const void *old_superblock,
                                      metadata_file_t::write_txn_t *out,
                                      signal_t *interruptor) {
    const cluster_metadata_superblock_t *sb =
        static_cast<const cluster_metadata_superblock_t *>(old_superblock);
    cluster_version_t v = cluster_superblock_version(sb);
    metadata_v1_16::cluster_semilattice_metadata_t metadata;

    if (v == cluster_version_t::v1_14 || v == cluster_version_t::v1_15) {
        metadata_v1_14::cluster_semilattice_metadata_t old_metadata;
        read_blob(buf_parent, sb->metadata_blob,
                  cluster_metadata_superblock_t::METADATA_BLOB_MAXREFLEN,
                  [&](read_stream_t *s) -> archive_result_t {
                      switch(v) {
                      case cluster_version_t::v1_14:
                          return deserialize<cluster_version_t::v1_14>(s, &old_metadata);
                      case cluster_version_t::v1_15:
                          return deserialize<cluster_version_t::v1_15>(s, &old_metadata);
                      case cluster_version_t::v1_16:
                      case cluster_version_t::v2_0:
                      case cluster_version_t::v2_1:
                      case cluster_version_t::v2_2_is_latest:
                      default:
                        unreachable();
                      }
                  });
        metadata = migrate_cluster_metadata_v1_14_to_v1_16(old_metadata);
    } else if (v == cluster_version_t::v1_16 || v == cluster_version_t::v2_0) {
        read_blob(buf_parent, sb->metadata_blob,
                  cluster_metadata_superblock_t::METADATA_BLOB_MAXREFLEN,
                  [&](read_stream_t *s) -> archive_result_t {
                      switch(v) {
                      case cluster_version_t::v1_16:
                          return deserialize<cluster_version_t::v1_16>(s, &metadata);
                      case cluster_version_t::v2_0:
                          return deserialize<cluster_version_t::v2_0>(s, &metadata);
                      case cluster_version_t::v1_14:
                      case cluster_version_t::v1_15:
                      case cluster_version_t::v2_1:
                      case cluster_version_t::v2_2_is_latest:
                      default:
                        unreachable();
                      }
                  });
    } else {
        unreachable();
    }

    // Check for obsolete sindexes before anything else because we don't want to
    // migrate half the tables and leave the user with an unusable directory.
    check_for_obsolete_sindexes(io_backender, base_path, metadata, interruptor);

    metadata_v1_16::branch_history_t branch_history;
    read_blob(buf_parent, sb->rdb_branch_history_blob,
              cluster_metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN,
              [&](read_stream_t *s) -> archive_result_t {
                  return deserialize_for_version(v, s, &branch_history);
              });

    migrate_heartbeat(out, interruptor);
    migrate_server(sb->server_id, metadata, out, interruptor);
    migrate_databases(metadata, out, interruptor);
    migrate_tables(io_backender, base_path,
                   sb->server_id, metadata, branch_history, out, interruptor);
}

void migrate_auth_metadata_to_v2_2(io_backender_t *io_backender,
                                   const serializer_filepath_t &path,
                                   metadata_file_t::write_txn_t *out,
                                   signal_t *interruptor) {
    logNTC("Migrating auth metadata");
    perfmon_collection_t dummy_stats;
    filepath_file_opener_t file_opener(path, io_backender);
    log_serializer_t serializer(log_serializer_t::dynamic_config_t(), &file_opener, &dummy_stats);

    if (!serializer.coop_lock_and_check()) {
        throw file_in_use_exc_t();
    }

    dummy_cache_balancer_t balancer(MEGABYTE);
    cache_t cache(&serializer, &balancer, &dummy_stats);
    cache_conn_t cache_conn(&cache);

    txn_t read_txn(&cache_conn, read_access_t::read);
    buf_lock_t superblock(buf_parent_t(&read_txn), SUPERBLOCK_ID, access_t::read);
    buf_read_t sb_read(&superblock);

    const auth_metadata_superblock_t *sb =
        static_cast<const auth_metadata_superblock_t *>(sb_read.get_data_read());
    cluster_version_t v = auth_superblock_version(sb);
    metadata_v1_16::auth_semilattice_metadata_t metadata;

    if (v == cluster_version_t::v1_14 || v == cluster_version_t::v1_15) {
        metadata_v1_14::auth_semilattice_metadata_t old_metadata;
        read_blob(buf_parent_t(&superblock),
                  sb->metadata_blob,
                  auth_metadata_superblock_t::METADATA_BLOB_MAXREFLEN,
                  [&](read_stream_t *s) -> archive_result_t {
                      switch (v) {
                      case cluster_version_t::v1_14:
                          return deserialize<cluster_version_t::v1_14>(s, &old_metadata);
                      case cluster_version_t::v1_15:
                          return deserialize<cluster_version_t::v1_15>(s, &old_metadata);
                      case cluster_version_t::v1_16:
                      case cluster_version_t::v2_0:
                      case cluster_version_t::v2_1:
                      case cluster_version_t::v2_2_is_latest:
                      default:
                          unreachable();
                      }
                  });
        metadata = migrate_auth_metadata_v1_14_to_v1_16(old_metadata);
    } else  if (v == cluster_version_t::v1_16 || v == cluster_version_t::v2_0) {
        read_blob(buf_parent_t(&superblock),
                  sb->metadata_blob,
                  auth_metadata_superblock_t::METADATA_BLOB_MAXREFLEN,
                  [&](read_stream_t *s) -> archive_result_t {
                      switch (v) {
                      case cluster_version_t::v1_16:
                          return deserialize<cluster_version_t::v1_16>(s, &metadata);
                      case cluster_version_t::v2_0:
                          return deserialize<cluster_version_t::v2_0>(s, &metadata);
                      case cluster_version_t::v1_14:
                      case cluster_version_t::v1_15:
                      case cluster_version_t::v2_1:
                      case cluster_version_t::v2_2_is_latest:
                      default:
                          unreachable();
                      }
                  });
    } else {
        unreachable();
    }

    // These structures are currently identical, although that could change in the future
    ::auth_semilattice_metadata_t new_metadata;
    new_metadata.auth_key = metadata.auth_key;

    out->write(mdkey_auth_semilattices(), new_metadata, interruptor);
}
