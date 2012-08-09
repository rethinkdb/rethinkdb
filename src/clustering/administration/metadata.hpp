#ifndef CLUSTERING_ADMINISTRATION_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_METADATA_HPP_

#include <list>
#include <map>
#include <string>
#include <vector>

// TODO: Probably some of these headers could be moved to the .cc.
#include "clustering/administration/datacenter_metadata.hpp"
#include "clustering/administration/database_metadata.hpp"
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
    databases_semilattice_metadata_t databases;

    RDB_MAKE_ME_SERIALIZABLE_6(dummy_namespaces, memcached_namespaces, rdb_namespaces, machines, datacenters, databases);
};

RDB_MAKE_SEMILATTICE_JOINABLE_6(cluster_semilattice_metadata_t, dummy_namespaces, memcached_namespaces, rdb_namespaces, machines, datacenters, databases);
RDB_MAKE_EQUALITY_COMPARABLE_6(cluster_semilattice_metadata_t, dummy_namespaces, memcached_namespaces, rdb_namespaces, machines, datacenters, databases);

//json adapter concept for cluster_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(cluster_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
cJSON *with_ctx_render_as_json(cluster_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_apply_json_to(cJSON *change, cluster_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_on_subfield_change(cluster_semilattice_metadata_t *, const vclock_ctx_t &);

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

// ctx-less json adapter for directory_echo_wrapper_t
template <typename T>
json_adapter_if_t::json_adapter_map_t get_json_subfields(directory_echo_wrapper_t<T> *target) {
    return get_json_subfields(&target->internal);
}

template <typename T>
cJSON *render_as_json(directory_echo_wrapper_t<T> *target) {
    return render_as_json(&target->internal);
}

template <typename T>
void apply_json_to(cJSON *change, directory_echo_wrapper_t<T> *target) {
    apply_json_to(change, &target->internal);
}

template <typename T>
void on_subfield_change(directory_echo_wrapper_t<T> *target) {
    on_subfield_change(&target->internal);
}


// ctx-less json adapter concept for cluster_directory_metadata_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(cluster_directory_metadata_t *target);
cJSON *render_as_json(cluster_directory_metadata_t *target);
void apply_json_to(cJSON *change, cluster_directory_metadata_t *target);
void on_subfield_change(cluster_directory_metadata_t *);


// ctx-less json adapter for cluster_directory_peer_type_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(cluster_directory_peer_type_t *);
cJSON *render_as_json(cluster_directory_peer_type_t *peer_type);
void apply_json_to(cJSON *, cluster_directory_peer_type_t *);
void on_subfield_change(cluster_directory_peer_type_t *);

#endif  // CLUSTERING_ADMINISTRATION_METADATA_HPP_
