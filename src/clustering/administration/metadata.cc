// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/metadata.hpp"

#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/datacenter_metadata.hpp"
#include "clustering/administration/machine_metadata.hpp"
#include "region/region_map_json_adapter.hpp"
#include "rdb_protocol/protocol.hpp"
#include "stl_utils.hpp"

RDB_IMPL_ME_SERIALIZABLE_1(database_semilattice_metadata_t, 0, name);
RDB_IMPL_SEMILATTICE_JOINABLE_1(database_semilattice_metadata_t, name);
RDB_IMPL_EQUALITY_COMPARABLE_1(database_semilattice_metadata_t, name);

RDB_IMPL_ME_SERIALIZABLE_1(databases_semilattice_metadata_t, 0, databases);
RDB_IMPL_SEMILATTICE_JOINABLE_1(databases_semilattice_metadata_t, databases);
RDB_IMPL_EQUALITY_COMPARABLE_1(databases_semilattice_metadata_t, databases);

RDB_IMPL_ME_SERIALIZABLE_1(datacenter_semilattice_metadata_t, 0, name);
RDB_IMPL_SEMILATTICE_JOINABLE_1(datacenter_semilattice_metadata_t, name);
RDB_IMPL_EQUALITY_COMPARABLE_1(datacenter_semilattice_metadata_t, name);

RDB_IMPL_ME_SERIALIZABLE_1(datacenters_semilattice_metadata_t, 0, datacenters);
RDB_IMPL_SEMILATTICE_JOINABLE_1(datacenters_semilattice_metadata_t, datacenters);
RDB_IMPL_EQUALITY_COMPARABLE_1(datacenters_semilattice_metadata_t, datacenters);

RDB_IMPL_ME_SERIALIZABLE_2(machine_semilattice_metadata_t, 0, datacenter, name);
RDB_IMPL_SEMILATTICE_JOINABLE_2(machine_semilattice_metadata_t, datacenter, name);
RDB_IMPL_EQUALITY_COMPARABLE_2(machine_semilattice_metadata_t, datacenter, name);

RDB_IMPL_ME_SERIALIZABLE_1(machines_semilattice_metadata_t, 0, machines);
RDB_IMPL_SEMILATTICE_JOINABLE_1(machines_semilattice_metadata_t, machines);
RDB_IMPL_EQUALITY_COMPARABLE_1(machines_semilattice_metadata_t, machines);

RDB_IMPL_ME_SERIALIZABLE_2(ack_expectation_t, 0, expectation_, hard_durability_);


RDB_IMPL_SEMILATTICE_JOINABLE_10(
        namespace_semilattice_metadata_t,
        blueprint, primary_datacenter, replica_affinities, ack_expectations, shards,
        name, primary_pinnings, secondary_pinnings, primary_key, database);

RDB_IMPL_EQUALITY_COMPARABLE_10(
        namespace_semilattice_metadata_t,
        blueprint, primary_datacenter, replica_affinities, ack_expectations, shards,
        name, primary_pinnings, secondary_pinnings, primary_key, database);

RDB_IMPL_SEMILATTICE_JOINABLE_1(namespaces_semilattice_metadata_t, namespaces);
RDB_IMPL_EQUALITY_COMPARABLE_1(namespaces_semilattice_metadata_t, namespaces);



namespace_semilattice_metadata_t new_namespace(
    uuid_u machine, uuid_u database, uuid_u datacenter,
    const name_string_t &name, const std::string &key) {

    namespace_semilattice_metadata_t ns;
    ns.database           = make_vclock(database, machine);
    ns.primary_datacenter = make_vclock(datacenter, machine);
    ns.name               = make_vclock(name, machine);
    ns.primary_key        = make_vclock(key, machine);

    std::map<uuid_u, ack_expectation_t> ack_expectations;
    ack_expectations[datacenter] = ack_expectation_t(1, true);
    ns.ack_expectations = make_vclock(ack_expectations, machine);

    nonoverlapping_regions_t shards;
    bool add_region_success = shards.add_region(region_t::universe());
    guarantee(add_region_success);
    ns.shards = make_vclock(shards, machine);

    region_map_t<uuid_u> primary_pinnings(region_t::universe(), nil_uuid());
    ns.primary_pinnings = make_vclock(primary_pinnings, machine);

    region_map_t<std::set<uuid_u> > secondary_pinnings(
        region_t::universe(), std::set<machine_id_t>());
    ns.secondary_pinnings = make_vclock(secondary_pinnings, machine);

    return ns;
}


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
    res["name"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<name_string_t>(&target->name, ctx));

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
    res["rdb_namespaces"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<cow_ptr_t<namespaces_semilattice_metadata_t>, vclock_ctx_t>(&target->rdb_namespaces, ctx));
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
    case ADMIN_PEER:
        return cJSON_CreateString("admin");
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


//json adapter concept for namespace_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(namespace_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["blueprint"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_read_only_adapter_t<vclock_t<persistable_blueprint_t>, vclock_ctx_t>(&target->blueprint, ctx));
    res["primary_uuid"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<datacenter_id_t>(&target->primary_datacenter, ctx));
    res["replica_affinities"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<std::map<datacenter_id_t, int32_t> >(&target->replica_affinities, ctx));
    res["ack_expectations"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<std::map<datacenter_id_t, ack_expectation_t> >(&target->ack_expectations, ctx));
    res["name"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<name_string_t>(&target->name, ctx));
    res["shards"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<nonoverlapping_regions_t>(&target->shards, ctx));
    res["primary_pinnings"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<region_map_t<machine_id_t> >(&target->primary_pinnings, ctx));
    res["secondary_pinnings"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<region_map_t<std::set<machine_id_t> > >(&target->secondary_pinnings, ctx));
    res["primary_key"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<std::string>(&target->primary_key, ctx));
    res["database"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<database_id_t>(&target->database, ctx));
    return res;
}

cJSON *with_ctx_render_as_json(namespace_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

void with_ctx_apply_json_to(cJSON *change, namespace_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

void with_ctx_on_subfield_change(namespace_semilattice_metadata_t *, const vclock_ctx_t &) { }

// json adapter concept for namespaces_semilattice_metadata_t
inline json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(namespaces_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    namespace_semilattice_metadata_t default_namespace;

    nonoverlapping_regions_t default_shards;
    bool success = default_shards.add_region(region_t::universe());
    guarantee(success);
    default_namespace.shards = default_namespace.shards.make_new_version(default_shards, ctx.us);

    /* It's important to initialize this because otherwise it will be
    initialized with a default-constructed UUID, which doesn't initialize its
    contents, so Valgrind will complain. */
    region_map_t<machine_id_t> default_primary_pinnings(region_t::universe(), nil_uuid());
    default_namespace.primary_pinnings = default_namespace.primary_pinnings.make_new_version(default_primary_pinnings, ctx.us);

    default_namespace.database = default_namespace.database.make_new_version(nil_uuid(), ctx.us);
    default_namespace.primary_datacenter = default_namespace.primary_datacenter.make_new_version(nil_uuid(), ctx.us);

    default_namespace.primary_key = default_namespace.primary_key.make_new_version("id", ctx.us);

    deletable_t<namespace_semilattice_metadata_t> default_ns_in_deletable(default_namespace);
    return json_ctx_adapter_with_inserter_t<namespaces_semilattice_metadata_t::namespace_map_t, vclock_ctx_t>(&target->namespaces, generate_uuid, ctx, default_ns_in_deletable).get_subfields();
}

cJSON *with_ctx_render_as_json(namespaces_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return with_ctx_render_as_json(&target->namespaces, ctx);
}

void with_ctx_apply_json_to(cJSON *change, namespaces_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, &target->namespaces, ctx);
}

void with_ctx_on_subfield_change(namespaces_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    with_ctx_on_subfield_change(&target->namespaces, ctx);
}

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

