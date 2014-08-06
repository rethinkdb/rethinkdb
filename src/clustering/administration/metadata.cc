// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/metadata.hpp"

#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/servers/datacenter_metadata.hpp"
#include "clustering/administration/servers/machine_metadata.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/cow_ptr_type.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/archive/versioned.hpp"
#include "rdb_protocol/protocol.hpp"
#include "region/region_map_json_adapter.hpp"
#include "stl_utils.hpp"

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(database_semilattice_metadata_t, name);
RDB_IMPL_SEMILATTICE_JOINABLE_1(database_semilattice_metadata_t, name);
RDB_IMPL_EQUALITY_COMPARABLE_1(database_semilattice_metadata_t, name);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(databases_semilattice_metadata_t, databases);
RDB_IMPL_SEMILATTICE_JOINABLE_1(databases_semilattice_metadata_t, databases);
RDB_IMPL_EQUALITY_COMPARABLE_1(databases_semilattice_metadata_t, databases);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(datacenter_semilattice_metadata_t, name);
RDB_IMPL_SEMILATTICE_JOINABLE_1(datacenter_semilattice_metadata_t, name);
RDB_IMPL_EQUALITY_COMPARABLE_1(datacenter_semilattice_metadata_t, name);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(datacenters_semilattice_metadata_t, datacenters);
RDB_IMPL_SEMILATTICE_JOINABLE_1(datacenters_semilattice_metadata_t, datacenters);
RDB_IMPL_EQUALITY_COMPARABLE_1(datacenters_semilattice_metadata_t, datacenters);

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_13(machine_semilattice_metadata_t, datacenter, name);
RDB_IMPL_SEMILATTICE_JOINABLE_2(machine_semilattice_metadata_t, datacenter, name);
RDB_IMPL_EQUALITY_COMPARABLE_2(machine_semilattice_metadata_t, datacenter, name);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(machines_semilattice_metadata_t, machines);
RDB_IMPL_SEMILATTICE_JOINABLE_1(machines_semilattice_metadata_t, machines);
RDB_IMPL_EQUALITY_COMPARABLE_1(machines_semilattice_metadata_t, machines);

RDB_IMPL_ME_SERIALIZABLE_2_SINCE_v1_13(ack_expectation_t, expectation_, hard_durability_);

RDB_IMPL_SERIALIZABLE_3(table_config_t::shard_t,
                        split_point, replica_names, director_names);
template void serialize<cluster_version_t::v1_14_is_latest>(
            write_message_t *, const table_config_t::shard_t &);
template archive_result_t deserialize<cluster_version_t::v1_14_is_latest>(
            read_stream_t *, table_config_t::shard_t *);
RDB_IMPL_EQUALITY_COMPARABLE_3(table_config_t::shard_t,
                               split_point, replica_names, director_names);

RDB_IMPL_SERIALIZABLE_1(table_config_t, shards);
template void serialize<cluster_version_t::v1_14_is_latest>(
            write_message_t *, const table_config_t &);
template archive_result_t deserialize<cluster_version_t::v1_14_is_latest>(
            read_stream_t *, table_config_t *);
RDB_IMPL_EQUALITY_COMPARABLE_1(table_config_t, shards);

RDB_IMPL_SERIALIZABLE_2(table_replication_info_t, config, chosen_directors);
template void serialize<cluster_version_t::v1_14_is_latest>(
            write_message_t *, const table_replication_info_t &);
template archive_result_t deserialize<cluster_version_t::v1_14_is_latest>(
            read_stream_t *, table_replication_info_t *);
RDB_IMPL_EQUALITY_COMPARABLE_2(table_replication_info_t, config, chosen_directors);

RDB_IMPL_SERIALIZABLE_4(namespace_semilattice_metadata_t,
                        name, database, primary_key, replication_info);
template void serialize<cluster_version_t::v1_14_is_latest>(
            write_message_t *, const namespace_semilattice_metadata_t &);
template archive_result_t deserialize<cluster_version_t::v1_14_is_latest>(
            read_stream_t *, namespace_semilattice_metadata_t *);

RDB_IMPL_SEMILATTICE_JOINABLE_4(
        namespace_semilattice_metadata_t,
        name, database, primary_key, replication_info);
RDB_IMPL_EQUALITY_COMPARABLE_4(
        namespace_semilattice_metadata_t,
        name, database, primary_key, replication_info);

RDB_IMPL_SERIALIZABLE_1(namespaces_semilattice_metadata_t, namespaces);
template void serialize<cluster_version_t::v1_14_is_latest>(
            write_message_t *, const namespaces_semilattice_metadata_t &);
template archive_result_t deserialize<cluster_version_t::v1_14_is_latest>(
            read_stream_t *, namespaces_semilattice_metadata_t *);
RDB_IMPL_SEMILATTICE_JOINABLE_1(namespaces_semilattice_metadata_t, namespaces);
RDB_IMPL_EQUALITY_COMPARABLE_1(namespaces_semilattice_metadata_t, namespaces);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(namespaces_directory_metadata_t, reactor_bcards);
RDB_IMPL_EQUALITY_COMPARABLE_1(namespaces_directory_metadata_t, reactor_bcards);

RDB_IMPL_SERIALIZABLE_4(
        cluster_semilattice_metadata_t, rdb_namespaces, machines, datacenters,
        databases);
template void serialize<cluster_version_t::v1_14_is_latest>(
            write_message_t *, const cluster_semilattice_metadata_t &);
template archive_result_t deserialize<cluster_version_t::v1_14_is_latest>(
            read_stream_t *, cluster_semilattice_metadata_t *);
RDB_IMPL_SEMILATTICE_JOINABLE_4(cluster_semilattice_metadata_t,
                                rdb_namespaces, machines, datacenters, databases);
RDB_IMPL_EQUALITY_COMPARABLE_4(cluster_semilattice_metadata_t,
                               rdb_namespaces, machines, datacenters, databases);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(auth_semilattice_metadata_t, auth_key);
RDB_IMPL_SEMILATTICE_JOINABLE_1(auth_semilattice_metadata_t, auth_key);
RDB_IMPL_EQUALITY_COMPARABLE_1(auth_semilattice_metadata_t, auth_key);

RDB_IMPL_SERIALIZABLE_10(cluster_directory_metadata_t,
                         rdb_namespaces, machine_id, peer_id, cache_size, ips,
                         get_stats_mailbox_address, log_mailbox,
                         server_name_business_card, local_issues, peer_type);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(cluster_directory_metadata_t);

bool ack_expectation_t::operator==(ack_expectation_t other) const {
    return expectation_ == other.expectation_ && hard_durability_ == other.hard_durability_;
}

void debug_print(printf_buffer_t *buf, const ack_expectation_t &x) {
    buf->appendf("ack_expectation{durability=%s, acks=%" PRIu32 "}",
                 x.is_hardly_durable() ? "hard" : "soft", x.expectation());
}

// json adapter concept for ack_expectation_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(ack_expectation_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["expectation"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<uint32_t>(&target->expectation_));
    res["hard_durability"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<bool>(&target->hard_durability_));
    return res;
}
cJSON *render_as_json(ack_expectation_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *change, ack_expectation_t *target) {
    apply_as_directory(change, target);
}

//json adapter concept for machine_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(machine_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["datacenter_uuid"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<datacenter_id_t>(&target->datacenter, ctx));
    res["name"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_read_only_adapter_t<vclock_t<name_string_t>, vclock_ctx_t>(&target->name, ctx));

    return res;
}

cJSON *with_ctx_render_as_json(machine_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

void with_ctx_apply_json_to(cJSON *change, machine_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

void with_ctx_on_subfield_change(machine_semilattice_metadata_t *, const vclock_ctx_t &) { }



//json adapter concept for machines_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(machines_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return with_ctx_get_json_subfields(&target->machines, ctx);
}

cJSON *with_ctx_render_as_json(machines_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return with_ctx_render_as_json(&target->machines, ctx);
}

void with_ctx_apply_json_to(cJSON *change, machines_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    with_ctx_apply_json_to(change, &target->machines, ctx);
}

void with_ctx_on_subfield_change(machines_semilattice_metadata_t *, const vclock_ctx_t &) { }


//json adapter concept for datacenter_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(datacenter_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["name"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<name_string_t>(&target->name, ctx));
    return res;
}

cJSON *with_ctx_render_as_json(datacenter_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

void with_ctx_apply_json_to(cJSON *change, datacenter_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

void with_ctx_on_subfield_change(datacenter_semilattice_metadata_t *, const vclock_ctx_t &) { }



//json adapter concept for datacenters_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(datacenters_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return json_ctx_adapter_with_inserter_t<datacenters_semilattice_metadata_t::datacenter_map_t, vclock_ctx_t>(&target->datacenters, generate_uuid, ctx).get_subfields();
}

cJSON *with_ctx_render_as_json(datacenters_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return with_ctx_render_as_json(&target->datacenters, ctx);
}

void with_ctx_apply_json_to(cJSON *change, datacenters_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, &target->datacenters, ctx);
}

void with_ctx_on_subfield_change(datacenters_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    with_ctx_on_subfield_change(&target->datacenters, ctx);
}

// json adapter concept for database_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(database_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["name"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<name_string_t>(&target->name, ctx));
    return res;
}

cJSON *with_ctx_render_as_json(database_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

void with_ctx_apply_json_to(cJSON *change, database_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

void with_ctx_on_subfield_change(database_semilattice_metadata_t *, const vclock_ctx_t &) { }

//json adapter concept for databases_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(databases_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return json_ctx_adapter_with_inserter_t<databases_semilattice_metadata_t::database_map_t, vclock_ctx_t>(&target->databases, generate_uuid, ctx).get_subfields();
}

cJSON *with_ctx_render_as_json(databases_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return with_ctx_render_as_json(&target->databases, ctx);
}

void with_ctx_apply_json_to(cJSON *change, databases_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, &target->databases, ctx);
}

void with_ctx_on_subfield_change(databases_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    with_ctx_on_subfield_change(&target->databases, ctx);
}




//json adapter concept for cluster_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(cluster_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    /* res["rdb_namespaces"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<cow_ptr_t<namespaces_semilattice_metadata_t>, vclock_ctx_t>(&target->rdb_namespaces, ctx)); */
    res["machines"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<machines_semilattice_metadata_t, vclock_ctx_t>(&target->machines, ctx));
    res["datacenters"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<datacenters_semilattice_metadata_t, vclock_ctx_t>(&target->datacenters, ctx));
    res["databases"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<databases_semilattice_metadata_t, vclock_ctx_t>(&target->databases, ctx));
    res["me"] = boost::shared_ptr<json_adapter_if_t>(new json_temporary_adapter_t<uuid_u>(ctx.us));
    return res;
}

cJSON *with_ctx_render_as_json(cluster_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

void with_ctx_apply_json_to(cJSON *change, cluster_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

void with_ctx_on_subfield_change(cluster_semilattice_metadata_t *, const vclock_ctx_t &) { }


// json adapter concept for auth_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(auth_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["auth_key"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<auth_key_t>(&target->auth_key, ctx));
    return res;
}

cJSON *with_ctx_render_as_json(auth_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

void with_ctx_apply_json_to(cJSON *change, auth_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

void with_ctx_on_subfield_change(auth_semilattice_metadata_t *, const vclock_ctx_t &) { }


// ctx-less json adapter concept for cluster_directory_metadata_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(cluster_directory_metadata_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["rdb_namespaces"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<namespaces_directory_metadata_t>(&target->rdb_namespaces));
    res["machine_id"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<machine_id_t>(&target->machine_id)); // TODO: should be 'me'?
    res["peer_id"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<peer_id_t>(&target->peer_id));
    res["cache_size"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<uint64_t>(&target->cache_size));
    res["ips"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<std::vector<std::string> >(&target->ips));
    res["peer_type"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<cluster_directory_peer_type_t>(&target->peer_type));
    return res;
}

cJSON *render_as_json(cluster_directory_metadata_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *change, cluster_directory_metadata_t *target) {
    apply_as_directory(change, target);
}




// ctx-less json adapter for cluster_directory_peer_type_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(cluster_directory_peer_type_t *) {
    return std::map<std::string, boost::shared_ptr<json_adapter_if_t> >();
}

cJSON *render_as_json(cluster_directory_peer_type_t *peer_type) {
    switch (*peer_type) {
    case SERVER_PEER:
        return cJSON_CreateString("server");
    case PROXY_PEER:
        return cJSON_CreateString("proxy");
    default:
        break;
    }
    return cJSON_CreateString("unknown");
}

void apply_json_to(cJSON *, cluster_directory_peer_type_t *) { }

// ctx-less json adapter concept for namespaces_directory_metadata_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(namespaces_directory_metadata_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["reactor_bcards"] = boost::shared_ptr<json_adapter_if_t>(new json_read_only_adapter_t<std::map<namespace_id_t, directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > > >(&target->reactor_bcards));
    return res;
}

cJSON *render_as_json(namespaces_directory_metadata_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *change, namespaces_directory_metadata_t *target) {
    apply_as_directory(change, target);
}

