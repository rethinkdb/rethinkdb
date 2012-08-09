#include "clustering/administration/metadata.hpp"

//json adapter concept for cluster_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(cluster_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["dummy_namespaces"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t>, vclock_ctx_t>(&target->dummy_namespaces, ctx));
    res["memcached_namespaces"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<namespaces_semilattice_metadata_t<memcached_protocol_t>, vclock_ctx_t>(&target->memcached_namespaces, ctx));
    res["rdb_namespaces"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<namespaces_semilattice_metadata_t<rdb_protocol_t>, vclock_ctx_t>(&target->rdb_namespaces, ctx));
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

