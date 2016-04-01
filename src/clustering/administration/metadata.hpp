// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_METADATA_HPP_

#include <map>
#include <string>
#include <vector>
#include <utility>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "clustering/administration/auth/user.hpp"
#include "clustering/administration/auth/username.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/jobs/report.hpp"
#include "clustering/administration/logs/log_transfer.hpp"
#include "clustering/administration/servers/server_metadata.hpp"
#include "clustering/administration/stats/stat_manager.hpp"
#include "clustering/administration/tables/database_metadata.hpp"
#include "clustering/table_manager/table_metadata.hpp"
#include "arch/address.hpp"
#include "rpc/connectivity/peer_id.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/versioned.hpp"
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
    auth_semilattice_metadata_t()
        : m_users({
            {
                auth::username_t("admin"),
                versioned_t<boost::optional<auth::user_t>>(
                    boost::make_optional(auth::user_t(auth::admin_t())))
            }
        }) {
    }

    std::map<auth::username_t, versioned_t<boost::optional<auth::user_t>>> m_users;
};

RDB_DECLARE_SERIALIZABLE(auth_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(auth_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(auth_semilattice_metadata_t);

class heartbeat_semilattice_metadata_t {
public:
    heartbeat_semilattice_metadata_t()
        : heartbeat_timeout(10000) { }

    versioned_t<uint64_t> heartbeat_timeout;
};

RDB_DECLARE_SERIALIZABLE(heartbeat_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(heartbeat_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(heartbeat_semilattice_metadata_t);

enum cluster_directory_peer_type_t {
    SERVER_PEER,
    PROXY_PEER
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(cluster_directory_peer_type_t, int8_t, SERVER_PEER, PROXY_PEER);

/* `proc_directory_metadata_t` is a part of `cluster_directory_metadata_t` that doesn't
change after the server starts and is only used for displaying in `server_status`. The
reason it's in a separate structure is to keep `cluster_directory_metadata_t` from being
too complicated. */
class proc_directory_metadata_t {
public:
    std::string version;   /* server version string, e.g. "rethinkdb 1.X.Y ..." */
    microtime_t time_started;
    int64_t pid;   /* really a `pid_t`, but we need a platform-independent type */
    std::string hostname;
    uint16_t cluster_port, reql_port;
    boost::optional<uint16_t> http_admin_port;
    std::set<host_and_port_t> canonical_addresses;
    std::vector<std::string> argv;
};

RDB_DECLARE_SERIALIZABLE(proc_directory_metadata_t);

class cluster_directory_metadata_t {
public:
    cluster_directory_metadata_t() { }
    cluster_directory_metadata_t(
            server_id_t _server_id,
            peer_id_t _peer_id,
            const proc_directory_metadata_t &_proc,
            uint64_t _actual_cache_size_bytes,
            const multi_table_manager_bcard_t &mtmbc,
            const jobs_manager_business_card_t& _jobs_mailbox,
            const get_stats_mailbox_address_t& _stats_mailbox,
            const log_server_business_card_t &lmb,
            const local_issue_bcard_t &lib,
            const server_config_versioned_t &sc,
            const boost::optional<server_config_business_card_t> &scbc,
            cluster_directory_peer_type_t _peer_type) :
        server_id(_server_id),
        peer_id(_peer_id),
        proc(_proc),
        actual_cache_size_bytes(_actual_cache_size_bytes),
        multi_table_manager_bcard(mtmbc),
        jobs_mailbox(_jobs_mailbox),
        get_stats_mailbox_address(_stats_mailbox),
        log_mailbox(lmb),
        local_issue_bcard(lib),
        server_config(sc),
        server_config_business_card(scbc),
        peer_type(_peer_type) { }
    /* Move constructor */
    cluster_directory_metadata_t(const cluster_directory_metadata_t &) = default;
    cluster_directory_metadata_t &operator=(const cluster_directory_metadata_t &)
        = default;

    server_id_t server_id;

    /* this is redundant, since a directory entry is always associated with a peer */
    peer_id_t peer_id;

    /* This group of fields are for showing in `rethinkdb.server_status` */
    proc_directory_metadata_t proc;
    uint64_t actual_cache_size_bytes;   /* might be user-set or automatically picked */

    multi_table_manager_bcard_t multi_table_manager_bcard;
    jobs_manager_business_card_t jobs_mailbox;
    get_stats_mailbox_address_t get_stats_mailbox_address;
    log_server_business_card_t log_mailbox;
    local_issue_bcard_t local_issue_bcard;

    /* For proxies, `server_config` is meaningless and `server_config_business_card` is
    empty. */
    server_config_versioned_t server_config;
    boost::optional<server_config_business_card_t> server_config_business_card;

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
        admin_err_t *error_out);

#endif  // CLUSTERING_ADMINISTRATION_METADATA_HPP_
