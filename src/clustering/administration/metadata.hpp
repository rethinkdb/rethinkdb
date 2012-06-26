#ifndef CLUSTERING_ADMINISTRATION_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_METADATA_HPP_

#include "clustering/administration/datacenter_metadata.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/log_transfer.hpp"
#include "clustering/administration/machine_metadata.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "clustering/administration/stat_manager.hpp"
#include "clustering/administration/metadata_change_handler.hpp"
#include "http/json/json_adapter.hpp"
#include "memcached/protocol.hpp"
#include "memcached/protocol_json_adapter.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/serialize_macros.hpp"

class cluster_semilattice_metadata_t {
public:
    cluster_semilattice_metadata_t() { }

    namespaces_semilattice_metadata_t<mock::dummy_protocol_t> dummy_namespaces;
    namespaces_semilattice_metadata_t<memcached_protocol_t> memcached_namespaces;
    namespaces_semilattice_metadata_t<rdb_protocol_t> rdb_namespaces;

    machines_semilattice_metadata_t machines;
    datacenters_semilattice_metadata_t datacenters;

    RDB_MAKE_ME_SERIALIZABLE_5(dummy_namespaces, memcached_namespaces, rdb_namespaces, machines, datacenters);
};

RDB_MAKE_SEMILATTICE_JOINABLE_5(cluster_semilattice_metadata_t, dummy_namespaces, memcached_namespaces, rdb_namespaces, machines, datacenters);
RDB_MAKE_EQUALITY_COMPARABLE_5(cluster_semilattice_metadata_t, dummy_namespaces, memcached_namespaces, rdb_namespaces, machines, datacenters);

//json adapter concept for cluster_semilattice_metadata_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(cluster_semilattice_metadata_t *target, const ctx_t &ctx) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    res["dummy_namespaces"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t>, ctx_t>(&target->dummy_namespaces));
    res["memcached_namespaces"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<namespaces_semilattice_metadata_t<memcached_protocol_t>, ctx_t>(&target->memcached_namespaces));
    res["rdb_namespaces"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<namespaces_semilattice_metadata_t<rdb_protocol_t>, ctx_t>(&target->rdb_namespaces));
    res["machines"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<machines_semilattice_metadata_t, ctx_t>(&target->machines));
    res["datacenters"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<datacenters_semilattice_metadata_t, ctx_t>(&target->datacenters));
    res["me"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_temporary_adapter_t<boost::uuids::uuid, ctx_t>(ctx.us));
    return res;
}

template <class ctx_t>
cJSON *render_as_json(cluster_semilattice_metadata_t *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, cluster_semilattice_metadata_t *target, const ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class ctx_t>
void on_subfield_change(cluster_semilattice_metadata_t *, const ctx_t &) { }

enum cluster_directory_peer_type_t {
    ADMIN_PEER,
    SERVER_PEER,
    PROXY_PEER
};

ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(cluster_directory_peer_type_t);

class cluster_directory_metadata_t {
public:

    cluster_directory_metadata_t() { }
    cluster_directory_metadata_t(
            machine_id_t mid,
            const std::vector<std::string> &_ips,
            const get_stats_mailbox_address_t& _stats_mailbox,
            const metadata_change_handler_t<cluster_semilattice_metadata_t>::request_mailbox_t::address_t& _semilattice_change_mailbox,
            const log_server_business_card_t &lmb,
            cluster_directory_peer_type_t _peer_type) :
        machine_id(mid), ips(_ips), get_stats_mailbox_address(_stats_mailbox), semilattice_change_mailbox(_semilattice_change_mailbox), log_mailbox(lmb), peer_type(_peer_type) { }

    namespaces_directory_metadata_t<mock::dummy_protocol_t> dummy_namespaces;
    namespaces_directory_metadata_t<memcached_protocol_t> memcached_namespaces;
    namespaces_directory_metadata_t<rdb_protocol_t> rdb_namespaces;

    /* Tell the other peers what our machine ID is */
    machine_id_t machine_id;

    /* To tell everyone what our ips are. */
    std::vector<std::string> ips;

    get_stats_mailbox_address_t get_stats_mailbox_address;
    metadata_change_handler_t<cluster_semilattice_metadata_t>::request_mailbox_t::address_t semilattice_change_mailbox;
    log_server_business_card_t log_mailbox;
    std::list<local_issue_t> local_issues;
    cluster_directory_peer_type_t peer_type;

    RDB_MAKE_ME_SERIALIZABLE_10(dummy_namespaces, memcached_namespaces, rdb_namespaces, machine_id, ips, get_stats_mailbox_address, semilattice_change_mailbox, log_mailbox, local_issues, peer_type);
};

// json adapter concept for directory_echo_wrapper_t
template <typename T, class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(directory_echo_wrapper_t<T> *target, const ctx_t &ctx) {
    return get_json_subfields(&target->internal, ctx);
}

template <typename T, class ctx_t>
cJSON *render_as_json(directory_echo_wrapper_t<T> *target, const ctx_t &ctx) {
    return render_as_json(&target->internal, ctx);
}

template <typename T, class ctx_t>
void apply_json_to(cJSON *change, directory_echo_wrapper_t<T> *target, const ctx_t &ctx) {
    apply_json_to(change, &target->internal, ctx);
}

template <typename T, class ctx_t>
void on_subfield_change(directory_echo_wrapper_t<T> *target, const ctx_t &ctx) {
    on_subfield_change(&target->internal, ctx);
}

//  json adapter concept for cluster_directory_metadata_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(cluster_directory_metadata_t *target, const ctx_t &) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    res["dummy_namespaces"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_read_only_adapter_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, ctx_t>(&target->dummy_namespaces));
    res["memcached_namespaces"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<namespaces_directory_metadata_t<memcached_protocol_t>, ctx_t>(&target->memcached_namespaces));
    res["machine_id"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<machine_id_t, ctx_t>(&target->machine_id)); // TODO: should be 'me'?
    res["ips"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<std::vector<std::string>, ctx_t>(&target->ips));
    res["peer_type"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<cluster_directory_peer_type_t, ctx_t>(&target->peer_type));
    return res;
}

template <class ctx_t>
cJSON *render_as_json(cluster_directory_metadata_t *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, cluster_directory_metadata_t *target, const ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class ctx_t>
void on_subfield_change(cluster_directory_metadata_t *, const ctx_t &) { }

// json adapter for cluster_directory_peer_type_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(cluster_directory_peer_type_t *, const ctx_t &)
{
    return std::map<std::string, boost::shared_ptr<json_adapter_if_t<ctx_t> > >();
}

template <class ctx_t>
cJSON *render_as_json(cluster_directory_peer_type_t *peer_type, const ctx_t &)
{
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

template <class ctx_t>
void apply_json_to(cJSON *, cluster_directory_peer_type_t *, const ctx_t &)
{ }

template <class ctx_t>
void  on_subfield_change(cluster_directory_peer_type_t *, const ctx_t &)
{ }

#endif  // CLUSTERING_ADMINISTRATION_METADATA_HPP_
