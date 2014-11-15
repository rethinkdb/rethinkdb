#include "clustering/administration/pre_v1_16_metadata.hpp"

namespace pre_v1_16 {

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(persistable_blueprint_t, machines_roles);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(database_semilattice_metadata_t, name);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(databases_semilattice_metadata_t, databases);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(datacenter_semilattice_metadata_t, name);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(datacenters_semilattice_metadata_t, datacenters);
RDB_IMPL_SERIALIZABLE_2_SINCE_v1_13(machine_semilattice_metadata_t, datacenter, name);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(machines_semilattice_metadata_t, machines);
RDB_IMPL_ME_SERIALIZABLE_2_SINCE_v1_13(ack_expectation_t, expectation_, hard_durability_);
RDB_IMPL_SERIALIZABLE_10_SINCE_v1_13(namespace_semilattice_metadata_t,
                                     blueprint, primary_datacenter, replica_affinities,
                                     ack_expectations, shards, name, primary_pinnings,
                                     secondary_pinnings, primary_key, database);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(namespaces_semilattice_metadata_t, namespaces);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(namespaces_directory_metadata_t, reactor_bcards);
RDB_IMPL_SERIALIZABLE_4_SINCE_v1_13(
        cluster_semilattice_metadata_t, rdb_namespaces, machines, datacenters,
        databases);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(auth_semilattice_metadata_t, auth_key);

} // namespace pre_v1_16

template<class T>
T get_vclock_best(
        const pre_v1_16::vclock_t<T> &vclock,
        int *best_total_out) {
    /* If there are multiple versions in the vector clock, choose the one with the
    greatest sum of the vector clock versions. This is a heuristic that will tend to
    choose the most recently modified version. */
    int best_total = -1;
    old_t best_value;
    for (const typename vclock_t<T>::stamped_value_t &value : vclock.values) {
        int total = 0;
        for (const auto &pair : value.first) {
            total += pair.second;
        }
        if (total > best_total) {
            best_value = value.second;
        }
    }
    if (best_total_out != nullptr) {
        *best_total_out = best_total;
    }
    return best_value;
}

template<class old_t, class new_t>
versioned_t<new_t> migrate_vclock(
        const pre_v1_16::vclock_t<old_t> &vclock,
        const std::function<new_t(const old_t &)> &converter) {
    int best_total;
    old_t best_value = get_vclock_best(vclock, &best_total);
    /* Calculate the timestamp so that it will appear before any "real" timestamp, but
    still be ordered according to the sum of the vector clock versions. This means that
    if the user migrates two files, and one has a vector clock update that the other one
    lacks, the one with the vector clock update will end up with a larger `versioned_t`
    timestamp, so the right thing will happen. */
    time_t timestamp = std::numeric_limits<time_t>::min() + 1 + best_total;
    return versioned_t::make_with_manual_timestamp(timestamp, converter(best_value));
}

template<class T>
versioned_t<T> migrate_vclock(
        const pre_v1_16::vclock_t<T> &vclock) {
    return migrate_vclock(vclock, [](const T &v) { return v; });
}

template<class old_t, class new_t>
std::map<uuid_u, deletable_t<new_t> > migrate_map(
        const std::map<uuid_u, deletable_t<old_t> > &old_map,
        const std::function(new_t(const old_t &)> &converter) {
    std::map<uuid_u, deletable_t<new_t> > new_map;
    for (const auto &pair : old_map) {
        if (pair.second.is_deleted()) {
            new_map[pair.first].mark_deleted();
        } else {
            new_map.insert(std::make_pair(
                pair.first,
                deletable_t<new_t>(converter(pair.second))));
        }
    }
    return new_map;
}

database_semilattice_metadata_t migrate_database(
        const pre_v1_16::database_semilattice_metadata_t &old_md) {
    database_semilattice_metadata_t new_md;
    new_md.name = migrate_vclock(old_md.name,
        [](const name_string_t &old_name) -> name_string_t {
            if (old_name == name_string_t::guarantee_valid("rethinkdb")) {
                logWRN("Found an existing database named `rethinkdb` when migrating "
                    "metadata. Since `rethinkdb` is a reserved database name as of "
                    "v1.16, the existing database has been renamed to `rethinkdb_`.");
                return name_string_t::guarantee_valid("rethinkdb_");
            }
            return old_name;
        });
    return new_md;
}

databases_semilattice_metadata_t migrate_database(
        const pre_v1_16::databases_semilattice_metadata_t &old_md) {
    databases_semilattice_metadata_t new_md;
    new_md.databases = migrate_map(old_md.databases, &migrate_database);
    return new_md;
}

server_semilattice_metadata_t migrate_machine(
        const pre_v1_16::machine_semilattice_metadata_t &old_md,
        const pre_v1_16::datacenters_semilattice_metadata_t &datacenters) {
    server_semilattice_metadata_t new_md;
    new_md.name = migrate_vclock(old_md.name);
    new_md.tags = migrate_vclock(old_md.datacenter,
        [&](const datacenter_id_t &dc) -> std::set<name_string_t> {
            std::set<name_string_t> tags;
            tags.insert(name_string_t::guarantee_valid("default"));
            auto it = datacenters.datacenters.find(dc);
            if (it != datacenters.datacenters.end() && !it->second.is_deleted()) {
                tags.insert(get_vclock_best(it->second.get_ref().name, nullptr));
            }
            return tags;
        });
    return new_md;
}

servers_semilattice_metadata_t migrate_machines(
        const pre_v1_16::machines_semilattice_metadata_t &old_md,
        const pre_v1_16::datacenters_semilattice_metadata_t &datacenters) {
    servers_semilattice_metadata_t new_md;
    new_md.servers = migrate_map(old_md.machines,
        [&](const pre_v1_16::machine_semilattice_metadata_t &old_machine) {
            return migrate_machine(old_machine, datacenters);
        });
    return new_md;
}

write_ack_config_t::req_t migrate_ack_req(
        int num_acks,
        std::set<server_id_t> replicas_for_acks,
        const std::vector<table_config_t::shard_t> &config_shards,
        name_string_t table_name,
        name_string_t db_name,
        const std::string &where) {
    guarantee(num_acks != 0);
    write_ack_config_t::req_t req;
    req.replicas = replicas_for_acks;
    if (num_acks == 1) {
        req.mode = write_ack_config_t::mode_t::single;
    } else {
        int largest = 0;
        for (const table_config_t::shard_t &shard : config_shards) {
            int overlap = 0;
            for (const server_id_t &server : shard.replicas) {
                overlap += replicas_for_acks.count(server);
            }
            largest = std::max(largest, overlap);
        }
        int majority_equivalent = (largest + 2) / 2;
        if (num_acks != majority_equivalent) {
            logWRN("For table `%s.%s`, you requested %d write acks %s; however, as of "
                "v1.16, RethinkDB does not allow you to specify an arbitrary number of "
                "acks. Your ack setting has been translated to the closest equivalent "
                "setting, which is `majority`; this is equivalent to %d acks.",
                db_name.c_str(), table_name.c_str(), num_acks, where.c_str(),
                majority_equivalent);
        }
        req.mode = write_ack_config_t::mode_t::majority;
    }
}

std::set<server_id_t> get_servers_in_dc(
        const pre_v1_16::machines_semilattice_metadata_t &machines,
        const datacenter_id_t &dc) {
    std::set<server_id_t> servers;
    for (const auto &pair : machines.machines) {
        if (pair.second.is_deleted()) {
            continue;
        }
        datacenter_id_t server_dc = get_vclock_best(pair.second.datacenter, nullptr);
        if (server_dc == dc) {
            servers.insert(pair.first);
        }
    }
    return servers;
}

name_string_t get_name_of_dc(
        const pre_v1_16::datacenters_semilattice_metadata_t &datacenters,
        const datacenter_id_t &dc) {
    auto it = datacenters.find(dc);
    if (it == datacenters.end() || it->second.is_deleted()) {
        return name_string_t::guarantee_valid("__deleted_datacenter__");
    } else {
        return get_vclock_best(it->second.name, nullptr);
    }
}

namespace_semilattice_metadata_t migrate_table(
        const pre_v1_16::namespace_semilattice_metadata_t &old_md,
        /* We need the databases, datacenters, and machines as context */
        const pre_v1_16::databases_semilattice_metadata_t &databases,
        const pre_v1_16::datacenters_semilattice_metadata_t &datacenters,
        const pre_v1_16::machines_semilattice_metadata_t &machines) {
    namespace_semilattice_metadata_t new_md;

    /* Migrate the easy fields */
    new_md.name = migrate_vclock(old_md.name);
    new_md.database = migrate_vclock(old_md.database);
    new_md.primary_key = migrate_vclock(old_md.primary_key);

    /* Extract the table and database name for error message purposes */
    name_string_t table_name = new_md.name.get_ref();
    name_string_t db_name;
    auto it = databases.databases.find(new_md.database.get_ref());
    if (it != databases.databases.end() && !it->second.is_deleted()) {
        db_name = get_vclock_best(it->second.name, nullptr);
    } else {
        db_name = name_string_t::guarantee_valid("__deleted_database__");
    }

    table_replication_info_t repli_info;

    /* Extract the data we'll need for the more complicated translations. We store the
    vclock totals individually and we'll add them together at the end. */
    int blueprint_vclock_total;
    persistable_blueprint_t blueprint =
        get_vclock_best(old_md.blueprint, &blueprint_vclock_total);
    int acks_vclock_total;
    std::map<datacenter_id_t, pre_v1_16::ack_expectations_t> acks =
        get_vclock_best(old_md.ack_expectations, &acks_vclock_total);
    int primary_dc_vclock_total;
    datacenter_id_t primary_dc =
        get_vclock_best(old_md.primary_datacenter, &primary_dc_vclock_total);

    /* Use the same split points as before */
    std::set<store_key_t> split_points;
    for (const auto &pair : blueprint.machines_roles.begin()->first) {
        region_t region = pair.first;
        store_key_t key = region.inner.left;
        if (key != store_key_t::min()) {
            split_points.inert(key);
        }
    }
    repli_info.shard_scheme.split_points.insert(
        split_points.begin(), split_points.end());

    /* Translate the old blueprint directly into the new table config */
    repli_info.table_config.shards.resize(repli_info.shard_scheme.num_shards());
    std::set<server_id_t> all_relevant_servers;
    for (const auto &server_roles : blueprint.machines_roles) {
        guarantee(server_roles.second.size() == repli_info.shard_scheme.num_shards());
        for (int i = 0; i < repli_info.shard_scheme.num_shards(); ++i) {
            table_config_t::shard_t *s = &repli_info.table_config.shards[i];
            region_t r = hash_region_t<key_range_t>(
                repli_info.shard_scheme.get_shard_range(i));
            auto it = server_roles.second.find(r);
            blueprint_role_t role;
            if (it == server_roles.second.end()) {
                logERR("Metadata corruption detected when migrating to v1.16 format: "
                    "table `%s.%s` has different shard boundaries for different "
                    "servers.",
                    db_name.c_str(), table_name.c_str());
                /* This is a hack; if the server doesn't have any role for this exact
                shard, we'll just set it as a secondary. This will result in the user's
                data being replicated too many times but at least they won't lose any
                data. */
                role = blueprint_role_secondary;
            } else {
                role = it->second;
            }
            if (role == blueprint_role_primary) {
                if (!s->director.is_unset()) {
                    logERR("Metadata corruption detected when migrating to v1.16 "
                        "format: table `%s.%s` has two different servers listed as "
                        "primary for a single shard.",
                        db_name.c_str(), table_name.c_str());
                    /* Choose one director arbitrarily. It's not the end of the world. */
                }
                s->director = server_roles.first;
                s->replicas.insert(server_roles.first);
            } else if (role == blueprint_role_secondary) {
                s->replicas.insert(server_roles.first);
            }
            if (role != blueprint_role_nothing) {
                all_relevant_servers.insert(server_roles.first);
            }
        }
    }
    for (int i = 0; i < repli_info.shard_scheme.num_shards(); ++i) {
        table_config_t::shard_t *s = &repli_info.table_config.shards[i];
        if (s->director.is_unset()) {
            /* This is probably impossible unless the user was mucking around with
            `/ajax`, but fortunately there's a pretty simple translation that won't break
            anything. */
            s->director = nil_uuid();
        }
    }

    /* Translating the write acks is the hardest part. We often can't translate directly;
    in this case we issue a warning. */
    int num_general_acks = 0, num_specific_acks = 0;
    for (const auto &pair : acks) {
        if (pair.second == 0) [
            continue;
        }
        if (pair.first.is_nil()) {
            /* A nil entry means that we're requiring "general acks" from any datacenter,
            in addition to the "specific acks" from particular datacenters. */
            num_general_acks += pair.second.expectation_;
            continue;
        }
        std::set<server_id_t> ack_servers = get_servers_in_dc(machines, pair.first);
        if (std::includes(all_relevant_servers.begin(), all_relevant_servers.end(),
                          ack_servers.begin(), ack_servers.end())) {
            /* Special-case the situation where all the replicas are in the same
            datacenter, by bundling the specific ack requirement for that datacenter in
            with the general ack requirement. The reason for special-casing this is that
            if there are no specific acks, we can use the simplified syntax for write
            acks, which is more user-friendly. */
            num_general_acks += pair.second.expectation_;
            continue;
        }
        name_string_t dc_name = get_name_of_dc(datacenters, pair.first);
        write_ack_config_t::req_t req = migrate_ack_req(
            pair.second,
            ack_servers,
            repli_info.table_config.shards,
            table_name,
            db_name,
            strprintf("in datacenter `%s`", dc_name.c_str()));
        repli_info.write_ack_config.complex_reqs.push_back(req);
        num_specific_acks += pair.second.expectation_;
    }
    if (num_specific_acks == 0) {
        /* There are no specific acks, so we can use the abbreviated syntax */
        if (num_general_acks == 0) {
            /* This is possible if the user reduced the ack requirement to zero for some
            reason. It's equivalent to a single general ack. */
            repli_info.write_ack_config.mode = write_ack_config_t::mode_t::single;
        } else {
            repli_info.write_ack_config.mode = migrate_ack_req(
                num_general_acks,
                all_relevant_servers,
                repli_info.table_config.shards,
                table_name,
                db_name,
                "overall").mode;
        }
    } else {
        repli_info.write_ack_config.mode = write_ack_config_t::mode_t::complex;
        if (num_general_acks > 0) {
            write_ack_config_t::req_t req = migrate_ack_req(
                /* Pre-v1.16 ack requirements were non-overlapping, so N general acks
                means N additional acks after all the specific acks are satisfied. v1.16
                ack requirements are overlapping, so to be equivalent we have to add the
                number of general acks to the total number of specific acks. */
                num_specific_acks + num_general_acks,
                all_relevant_servers,
                repli_info.table_config.shards,
                table_name,
                db_name,
                "overall");
            write_ack_config.complex_reqs.insert(
                /* Put the general requiremenet at the beginning because it's more
                similar to how it was displayed in the web UI */
                write_ack_config.complex_reqs.begin(),
                req);
        }
    }

    /* Translate the hard/soft write durability setting. Usually we can translate it
    directly; other times we have to default to hard durability. */
    int num_soft = 0, num_hard = 0;
    for (const auto &pair : acks) {
        if (pair.second.hard_durability_) {
            ++num_hard;
        } else {
            ++num_soft;
        }
    }
    if (num_hard > 0 && num_soft > 0) {
        /* This is only possible if the user manually tweaked something through the CLI
        or by messing with the HTTP admin interface */
        logWRN("For table `%s.%s`, your pre-v1.16 settings specified hard write "
            "durability when writing to some servers and soft write durability for "
            "others. As of version 1.16, RethinkDB no longer allows mixed durability "
            "settings. Your table will now use hard durability everywhere. You can "
            "change this setting by writing to the `rethinkdb.table_config` table.",
            db_name.c_str(), table_name.c_str());
        repli_info.table_config.write_durability = write_durability_t::HARD;
    } else if (num_soft > 0) {
        repli_info.table_config.write_durability = write_durability_t::SOFT;
    } else {
        repli_info.table_config.write_durability = write_durability_t::HARD;
    }

    /* Write `repli_info` back to `new_md`, wrapped in a `versioned_t` */
    new_md.repli_info = versioned_t<table_repli_info_t>::make_with_manual_timestamp(
        /* Choose the timestamp by adding together the vclock totals from the fields on
        the pre-v1.16 metadata we consulted. This ensures that if we apply this procedure
        to two servers' metadata, and one has a more up-to-date vector clock, then the
        one with the more up-to-date vector clock will produce a `versioned_t` with a
        later timestamp. */ 
        std::numeric_limits<time_t>::min() + 1 +
            blueprint_vclock_total + acks_vclock_total + primary_dc_vclock_total,
        repli_info);

    return new_md;
}

namespaces_semilattice_metadata_t migrate_tables(
        const pre_v1_16::namespaces_semilattice_metadata_t &old_md,
        /* We need the databases, datacenters, and machines as context */
        const pre_v1_16::databases_semilattice_metadata_t &databases,
        const pre_v1_16::datacenters_semilattice_metadata_t &datacenters,
        const pre_v1_16::machines_semilattice_metadata_t &machines) {
    namespaces_semilattice_metadata_t new_md;
    new_md.namespaces = migrate_map(old_md.namespaces,
        [&](const pre_v1_16::namespace_semilattice_metadata_t &old_table) {
            return migrate_table(old_table, databases, datacenters, machines);
        });
    return new_md;
}

cluster_semilattice_metadata_t migrate_cluster_metadata_to_v1_16(
        const pre_v1_16::cluster_semilattice_metadata_t &old_md) {
    logINF("Migrating pre-v1.16 metadata to new format...");
    cluster_semilattice_metadata_t new_md;
    new_md.rdb_namespaces = cow_ptr_t<namespaces_semilattice_metadata_t>(
        migrate_tables(*old_md.rdb_namespaces,
            old_md.databases, old_md.datacenters, old_md.machines));
    new_md.servers = migrate_servers(old_md.machines, old_md.datacenters);
    new_md.databases = migrate_databases(old_md.databases);
    return new_md;
}

auth_semilattice_metadata_t migrate_auth_metadata_to_v1_16(
        const pre_v1_16::auth_semilattice_metadata_t &old_md) {
    auth_semilattice_metadata_t new_md;
    new_md.auth_key = migrate_vclock(old_md.auth_key);
    return new_md;
}

