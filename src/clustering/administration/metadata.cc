#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/datacenter_metadata.hpp"
#include "clustering/administration/machine_metadata.hpp"
#include "clustering/administration/metadata.hpp"

//json adapter concept for machine_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(machine_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["datacenter_uuid"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<datacenter_id_t>(&target->datacenter, ctx));
    res["name"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<std::string>(&target->name, ctx));

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
    res["name"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<std::string>(&target->name, ctx));
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
    res["name"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<std::string>(&target->name, ctx));
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
    res["dummy_namespaces"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >, vclock_ctx_t>(&target->dummy_namespaces, ctx));
    res["memcached_namespaces"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >, vclock_ctx_t>(&target->memcached_namespaces, ctx));
    res["rdb_namespaces"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >, vclock_ctx_t>(&target->rdb_namespaces, ctx));
    res["machines"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<machines_semilattice_metadata_t, vclock_ctx_t>(&target->machines, ctx));
    res["datacenters"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<datacenters_semilattice_metadata_t, vclock_ctx_t>(&target->datacenters, ctx));
    res["databases"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<databases_semilattice_metadata_t, vclock_ctx_t>(&target->databases, ctx));
    res["me"] = boost::shared_ptr<json_adapter_if_t>(new json_temporary_adapter_t<uuid_t>(ctx.us));
    return res;
}

cJSON *with_ctx_render_as_json(cluster_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

void with_ctx_apply_json_to(cJSON *change, cluster_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

void with_ctx_on_subfield_change(cluster_semilattice_metadata_t *, const vclock_ctx_t &) { }



// ctx-less json adapter concept for cluster_directory_metadata_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(cluster_directory_metadata_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["dummy_namespaces"] = boost::shared_ptr<json_adapter_if_t>(new json_read_only_adapter_t<namespaces_directory_metadata_t<mock::dummy_protocol_t> >(&target->dummy_namespaces));
    res["memcached_namespaces"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<namespaces_directory_metadata_t<memcached_protocol_t> >(&target->memcached_namespaces));
    res["rdb_namespaces"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<namespaces_directory_metadata_t<rdb_protocol_t> >(&target->rdb_namespaces));
    res["machine_id"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<machine_id_t>(&target->machine_id)); // TODO: should be 'me'?
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

void on_subfield_change(cluster_directory_metadata_t *) { }




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

void on_subfield_change(cluster_directory_peer_type_t *) { }


//json adapter concept for namespace_semilattice_metadata_t
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(namespace_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["blueprint"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_read_only_adapter_t<vclock_t<persistable_blueprint_t<protocol_t> >, vclock_ctx_t>(&target->blueprint, ctx));
    res["primary_uuid"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<datacenter_id_t>(&target->primary_datacenter, ctx));
    res["replica_affinities"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<std::map<datacenter_id_t, int> >(&target->replica_affinities, ctx));
    res["ack_expectations"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<std::map<datacenter_id_t, int> >(&target->ack_expectations, ctx));
    res["name"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<std::string>(&target->name, ctx));
    res["shards"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<std::set<typename protocol_t::region_t> >(&target->shards, ctx));
    res["port"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<int>(&target->port, ctx));
    res["primary_pinnings"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<region_map_t<protocol_t, machine_id_t> >(&target->primary_pinnings, ctx));
    res["secondary_pinnings"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<region_map_t<protocol_t, std::set<machine_id_t> > >(&target->secondary_pinnings, ctx));
    res["primary_key"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_read_only_adapter_t<vclock_t<std::string>, vclock_ctx_t>(&target->primary_key, ctx));
    res["database"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<database_id_t>(&target->database, ctx));
    return res;
}

template <class protocol_t>
cJSON *with_ctx_render_as_json(namespace_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class protocol_t>
void with_ctx_apply_json_to(cJSON *change, namespace_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class protocol_t>
void with_ctx_on_subfield_change(namespace_semilattice_metadata_t<protocol_t> *, const vclock_ctx_t &) { }

// json adapter concept for namespaces_semilattice_metadata_t
template <class protocol_t>
inline json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(namespaces_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx) {
    namespace_semilattice_metadata_t<protocol_t> default_namespace;

    std::set<typename protocol_t::region_t> default_shards;
    default_shards.insert(protocol_t::region_t::universe());
    default_namespace.shards = default_namespace.shards.make_new_version(default_shards, ctx.us);

    /* It's important to initialize this because otherwise it will be
    initialized with a default-constructed UUID, which doesn't initialize its
    contents, so Valgrind will complain. */
    region_map_t<protocol_t, machine_id_t> default_primary_pinnings(protocol_t::region_t::universe(), nil_uuid());
    default_namespace.primary_pinnings = default_namespace.primary_pinnings.make_new_version(default_primary_pinnings, ctx.us);

    default_namespace.database = default_namespace.database.make_new_version(nil_uuid(), ctx.us);

    default_namespace.primary_key = default_namespace.primary_key.make_new_version("id", ctx.us);

    deletable_t<namespace_semilattice_metadata_t<protocol_t> > default_ns_in_deletable(default_namespace);
    return json_ctx_adapter_with_inserter_t<typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t, vclock_ctx_t>(&target->namespaces, generate_uuid, ctx, default_ns_in_deletable).get_subfields();
}

template <class protocol_t>
cJSON *with_ctx_render_as_json(namespaces_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx) {
    return with_ctx_render_as_json(&target->namespaces, ctx);
}

template <class protocol_t>
void with_ctx_apply_json_to(cJSON *change, namespaces_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, &target->namespaces, ctx);
}

template <class protocol_t>
void with_ctx_on_subfield_change(namespaces_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx) {
    with_ctx_on_subfield_change(&target->namespaces, ctx);
}

// ctx-less json adapter concept for namespaces_directory_metadata_t
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(namespaces_directory_metadata_t<protocol_t> *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["reactor_bcards"] = boost::shared_ptr<json_adapter_if_t>(new json_read_only_adapter_t<std::map<namespace_id_t, directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > > >(&target->reactor_bcards));
    return res;
}

template <class protocol_t>
cJSON *render_as_json(namespaces_directory_metadata_t<protocol_t> *target) {
    return render_as_directory(target);
}

template <class protocol_t>
void apply_json_to(cJSON *change, namespaces_directory_metadata_t<protocol_t> *target) {
    apply_as_directory(change, target);
}

template <class protocol_t>
void on_subfield_change(UNUSED namespaces_directory_metadata_t<protocol_t> *target) { }



#include "memcached/protocol_json_adapter.hpp"
template json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields<memcached_protocol_t>(namespaces_semilattice_metadata_t<memcached_protocol_t> *target, const vclock_ctx_t &ctx);
template cJSON *with_ctx_render_as_json<memcached_protocol_t>(namespaces_semilattice_metadata_t<memcached_protocol_t> *target, const vclock_ctx_t &ctx);
template void with_ctx_apply_json_to<memcached_protocol_t>(cJSON *change, namespaces_semilattice_metadata_t<memcached_protocol_t> *target, const vclock_ctx_t &ctx);
template void with_ctx_on_subfield_change<memcached_protocol_t>(namespaces_semilattice_metadata_t<memcached_protocol_t> *, const vclock_ctx_t &);

template json_adapter_if_t::json_adapter_map_t get_json_subfields<memcached_protocol_t>(namespaces_directory_metadata_t<memcached_protocol_t> *target);
template cJSON *render_as_json<memcached_protocol_t>(namespaces_directory_metadata_t<memcached_protocol_t> *target);
template void apply_json_to<memcached_protocol_t>(cJSON *change, namespaces_directory_metadata_t<memcached_protocol_t> *target);
template void on_subfield_change<memcached_protocol_t>(UNUSED namespaces_directory_metadata_t<memcached_protocol_t> *target);


#include "rdb_protocol/protocol.hpp"
template json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields<rdb_protocol_t>(namespaces_semilattice_metadata_t<rdb_protocol_t> *target, const vclock_ctx_t &ctx);
template cJSON *with_ctx_render_as_json<rdb_protocol_t>(namespaces_semilattice_metadata_t<rdb_protocol_t> *target, const vclock_ctx_t &ctx);
template void with_ctx_apply_json_to<rdb_protocol_t>(cJSON *change, namespaces_semilattice_metadata_t<rdb_protocol_t> *target, const vclock_ctx_t &ctx);
template void with_ctx_on_subfield_change<rdb_protocol_t>(namespaces_semilattice_metadata_t<rdb_protocol_t> *, const vclock_ctx_t &);

template json_adapter_if_t::json_adapter_map_t get_json_subfields<rdb_protocol_t>(namespaces_directory_metadata_t<rdb_protocol_t> *target);
template cJSON *render_as_json<rdb_protocol_t>(namespaces_directory_metadata_t<rdb_protocol_t> *target);
template void apply_json_to<rdb_protocol_t>(cJSON *change, namespaces_directory_metadata_t<rdb_protocol_t> *target);
template void on_subfield_change<rdb_protocol_t>(UNUSED namespaces_directory_metadata_t<rdb_protocol_t> *target);


#include "mock/dummy_protocol_json_adapter.hpp"
template json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields<mock::dummy_protocol_t>(namespaces_semilattice_metadata_t<mock::dummy_protocol_t> *target, const vclock_ctx_t &ctx);
template cJSON *with_ctx_render_as_json<mock::dummy_protocol_t>(namespaces_semilattice_metadata_t<mock::dummy_protocol_t> *target, const vclock_ctx_t &ctx);
template void with_ctx_apply_json_to<mock::dummy_protocol_t>(cJSON *change, namespaces_semilattice_metadata_t<mock::dummy_protocol_t> *target, const vclock_ctx_t &ctx);
template void with_ctx_on_subfield_change<mock::dummy_protocol_t>(namespaces_semilattice_metadata_t<mock::dummy_protocol_t> *, const vclock_ctx_t &);

template json_adapter_if_t::json_adapter_map_t get_json_subfields<mock::dummy_protocol_t>(namespaces_directory_metadata_t<mock::dummy_protocol_t> *target);
template cJSON *render_as_json<mock::dummy_protocol_t>(namespaces_directory_metadata_t<mock::dummy_protocol_t> *target);
template void apply_json_to<mock::dummy_protocol_t>(cJSON *change, namespaces_directory_metadata_t<mock::dummy_protocol_t> *target);
template void on_subfield_change<mock::dummy_protocol_t>(UNUSED namespaces_directory_metadata_t<mock::dummy_protocol_t> *target);



