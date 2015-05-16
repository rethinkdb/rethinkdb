// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_METADATA_HPP_

#include <map>
#include <string>
#include <vector>
#include <utility>

// TODO: Probably some of these headers could be moved to the .cc.
#include "clustering/administration/issues/local_issue_aggregator.hpp"
#include "clustering/administration/jobs/manager.hpp"
#include "clustering/administration/logs/log_transfer.hpp"
#include "clustering/administration/servers/server_metadata.hpp"
#include "clustering/administration/stats/stat_manager.hpp"
#include "clustering/administration/tables/database_metadata.hpp"
#include "clustering/table_manager/table_metadata.hpp"
#include "containers/cow_ptr.hpp"
#include "containers/auth_key.hpp"
#include "rpc/semilattice/joins/cow_ptr.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/serialize_macros.hpp"


class cluster_semilattice_metadata_t {
public:
    cluster_semilattice_metadata_t() { }

    databases_semilattice_metadata_t databases;
};

RDB_DECLARE_SERIALIZABLE(cluster_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(cluster_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(cluster_semilattice_metadata_t);

class auth_semilattice_metadata_t {
public:
    auth_semilattice_metadata_t() { }

    versioned_t<auth_key_t> auth_key;
};

RDB_DECLARE_SERIALIZABLE(auth_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(auth_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(auth_semilattice_metadata_t);

enum cluster_directory_peer_type_t {
    SERVER_PEER,
    PROXY_PEER
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(cluster_directory_peer_type_t, int8_t, SERVER_PEER, PROXY_PEER);

class cluster_directory_metadata_t {
public:

    cluster_directory_metadata_t() { }
    cluster_directory_metadata_t(
            server_id_t _server_id,
            peer_id_t _peer_id,
            const std::string &_version,
            microtime_t _time_started,
            int64_t _pid,
            const std::string &_hostname,
            uint16_t _cluster_port,
            uint16_t _reql_port,
            boost::optional<uint16_t> _http_admin_port,
            std::set<host_and_port_t> _canonical_addresses,
            const std::vector<std::string> &_argv,
            uint64_t _actual_cache_size_bytes,
            const multi_table_manager_bcard_t &mtmbc,
            const jobs_manager_business_card_t& _jobs_mailbox,
            const get_stats_mailbox_address_t& _stats_mailbox,
            const log_server_business_card_t &lmb,
            const boost::optional<server_config_t> &sc,
            const boost::optional<server_config_business_card_t> &scbc,
            cluster_directory_peer_type_t _peer_type) :
        server_id(_server_id),
        peer_id(_peer_id),
        version(_version),
        time_started(_time_started),
        pid(_pid),
        hostname(_hostname),
        cluster_port(_cluster_port),
        reql_port(_reql_port),
        http_admin_port(_http_admin_port),
        canonical_addresses(_canonical_addresses),
        argv(_argv),
        actual_cache_size_bytes(_actual_cache_size_bytes),
        multi_table_manager_bcard(mtmbc),
        jobs_mailbox(_jobs_mailbox),
        get_stats_mailbox_address(_stats_mailbox),
        log_mailbox(lmb),
        server_config(sc),
        server_config_business_card(scbc),
        peer_type(_peer_type) { }
    /* Move constructor */
    cluster_directory_metadata_t(const cluster_directory_metadata_t &) = default;
    cluster_directory_metadata_t &operator=(const cluster_directory_metadata_t &)
        = default;
    cluster_directory_metadata_t(cluster_directory_metadata_t &&other) {
        *this = std::move(other);
    }
    cluster_directory_metadata_t &operator=(cluster_directory_metadata_t &&other) {
        /* We have to define this manually instead of using `= default` because older
        versions of `boost::optional` don't support moving */
        server_id = other.server_id;
        peer_id = other.peer_id;
        version = std::move(other.version);
        time_started = other.time_started;
        pid = other.pid;
        hostname = std::move(other.hostname);
        cluster_port = other.cluster_port;
        reql_port = other.reql_port;
        http_admin_port = other.http_admin_port;
        canonical_addresses = std::move(other.canonical_addresses);
        argv = std::move(other.argv);
        actual_cache_size_bytes = other.actual_cache_size_bytes;
        multi_table_manager_bcard = other.multi_table_manager_bcard;
        jobs_mailbox = other.jobs_mailbox;
        get_stats_mailbox_address = other.get_stats_mailbox_address;
        log_mailbox = other.log_mailbox;
        server_config = other.server_config;
        server_config_business_card = other.server_config_business_card;
        local_issues = std::move(other.local_issues);
        peer_type = other.peer_type;
        return *this;
    }

    server_id_t server_id;

    /* this is redundant, since a directory entry is always associated with a peer */
    peer_id_t peer_id;

    /* This group of fields are for showing in `rethinkdb.server_status` */
    std::string version;   /* server version string, e.g. "rethinkdb 1.X.Y ..." */
    microtime_t time_started;
    int64_t pid;   /* really a `pid_t`, but we need a platform-independent type */
    std::string hostname;
    uint16_t cluster_port, reql_port;
    boost::optional<uint16_t> http_admin_port;
    std::set<host_and_port_t> canonical_addresses;
    std::vector<std::string> argv;
    uint64_t actual_cache_size_bytes;   /* might be user-set or automatically picked */

    multi_table_manager_bcard_t multi_table_manager_bcard;
    jobs_manager_business_card_t jobs_mailbox;
    get_stats_mailbox_address_t get_stats_mailbox_address;
    log_server_business_card_t log_mailbox;
    boost::optional<server_config_t> server_config;
    boost::optional<server_config_business_card_t> server_config_business_card;
    local_issues_t local_issues;
    cluster_directory_peer_type_t peer_type;
};

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(cluster_directory_metadata_t);

template<class T>
bool search_metadata_by_uuid(
        std::map<uuid_u, deletable_t<T> > *map,
        const uuid_u &uuid,
        typename std::map<uuid_u, deletable_t<T> >::iterator *it_out) {
    auto it = map->find(uuid);
    if (it != map->end() && !it->second.is_deleted()) {
        *it_out = it;
        return true;
    } else {
        return false;
    }
}

template<class T>
bool search_const_metadata_by_uuid(
        const std::map<uuid_u, deletable_t<T> > *map,
        const uuid_u &uuid,
        typename std::map<uuid_u, deletable_t<T> >::const_iterator *it_out) {
    auto it = map->find(uuid);
    if (it != map->end() && !it->second.is_deleted()) {
        *it_out = it;
        return true;
    } else {
        return false;
    }
}

bool search_db_metadata_by_name(
        const databases_semilattice_metadata_t &metadata,
        const name_string_t &name,
        database_id_t *id_out,
        std::string *error_out);

#endif  // CLUSTERING_ADMINISTRATION_METADATA_HPP_
