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
#include "clustering/administration/log_transfer.hpp"
#include "clustering/administration/servers/server_metadata.hpp"
#include "clustering/administration/stats/stat_manager.hpp"
#include "clustering/administration/tables/database_metadata.hpp"
#include "clustering/administration/tables/table_metadata.hpp"
#include "containers/cow_ptr.hpp"
#include "containers/auth_key.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/cow_ptr.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/serialize_macros.hpp"


class cluster_semilattice_metadata_t {
public:
    cluster_semilattice_metadata_t() { }

    cow_ptr_t<namespaces_semilattice_metadata_t> rdb_namespaces;

    servers_semilattice_metadata_t servers;
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
            const jobs_manager_business_card_t& _jobs_mailbox,
            const get_stats_mailbox_address_t& _stats_mailbox,
            const log_server_business_card_t &lmb,
            const boost::optional<server_config_business_card_t> &nsbc,
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
        jobs_mailbox(_jobs_mailbox),
        get_stats_mailbox_address(_stats_mailbox),
        log_mailbox(lmb),
        server_config_business_card(nsbc),
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
        jobs_mailbox = other.jobs_mailbox;
        get_stats_mailbox_address = other.get_stats_mailbox_address;
        log_mailbox = other.log_mailbox;
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

    jobs_manager_business_card_t jobs_mailbox;
    get_stats_mailbox_address_t get_stats_mailbox_address;
    log_server_business_card_t log_mailbox;
    boost::optional<server_config_business_card_t> server_config_business_card;
    local_issues_t local_issues;
    cluster_directory_peer_type_t peer_type;
};

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(cluster_directory_metadata_t);

// ctx-less json adapter for directory_echo_wrapper_t
template <class T>
json_adapter_if_t::json_adapter_map_t get_json_subfields(directory_echo_wrapper_t<T> *target) {
    return get_json_subfields(&target->internal);
}

template <class T>
cJSON *render_as_json(directory_echo_wrapper_t<T> *target) {
    return render_as_json(&target->internal);
}

template <class T>
void apply_json_to(cJSON *change, directory_echo_wrapper_t<T> *target) {
    apply_json_to(change, &target->internal);
}

// ctx-less json adapter concept for cluster_directory_metadata_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(cluster_directory_metadata_t *target);
cJSON *render_as_json(cluster_directory_metadata_t *target);
void apply_json_to(cJSON *change, cluster_directory_metadata_t *target);


// ctx-less json adapter for cluster_directory_peer_type_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(cluster_directory_peer_type_t *);
cJSON *render_as_json(cluster_directory_peer_type_t *peer_type);
void apply_json_to(cJSON *, cluster_directory_peer_type_t *);

enum metadata_search_status_t {
    METADATA_SUCCESS, METADATA_ERR_NONE, METADATA_ERR_MULTIPLE
};

bool check_metadata_status(metadata_search_status_t status,
                           const char *entity_type,
                           const std::string &entity_name,
                           bool expect_present,
                           std::string *error_out);

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

/* A helper class to search through metadata in various ways.  Can be
   constructed from a pointer to the internal map of the metadata,
   e.g. `metadata.databases.databases`.  Look in rdb_protocol/query_language.cc
   for examples on how to use.
   `generic_metadata_searcher_t` should not be directly used. Instead there
   are two variants defined below:
     `const_metadata_searcher_t` for const maps
     and `metadata_searcher_t` for non-const maps. */
template<class T, class metamap_t, class iterator_t>
class generic_metadata_searcher_t {
public:
    typedef iterator_t iterator;
    iterator begin() {return map->begin();}
    iterator end() {return map->end();}

    explicit generic_metadata_searcher_t(metamap_t *_map): map(_map) { }

    template<class callable_t>
    /* Find the next iterator >= [start] matching [predicate]. */
    iterator find_next(iterator start, const callable_t& predicate) {
        iterator it;
        for (it = start; it != end(); ++it) {
            if (it->second.is_deleted()) continue;
            if (predicate(it->second.get_ref())) break;
        }
        return it;
    }
    /* Find the next iterator >= [start] (as above, but predicate always true). */
    iterator find_next(iterator start) {
        iterator it;
        for (it = start; it != end(); ++it) {
            if (!it->second.is_deleted()) {
                break;
            }
        }
        return it;
    }

    /* Find the unique entry matching [predicate].  If there is no unique entry,
       return [end()] and set the optional status parameter appropriately. */
    template<class callable_t>
    iterator find_uniq(const callable_t& predicate, metadata_search_status_t *out = 0) {
        iterator it, retval;
        if (out) *out = METADATA_SUCCESS;
        retval = it = find_next(begin(), predicate);
        if (it == end()) {
            if (out) *out = METADATA_ERR_NONE;
        } else if (find_next(++it, predicate) != end()) {
            if (out) *out = METADATA_ERR_MULTIPLE;
            retval = end();
        }
        return retval;
    }
    /* As above, but matches by name instead of a predicate. */
    iterator find_uniq(const name_string_t &name, metadata_search_status_t *out = 0) {
        return find_uniq(name_predicate_t(&name), out);
    }

    struct name_predicate_t {
        bool operator()(T metadata) const {
            return metadata.name.get_ref() == *name;
        }
        explicit name_predicate_t(const name_string_t *_name): name(_name) { }
    private:
        const name_string_t *name;
    };
private:
    metamap_t *map;
};
template<class T>
class metadata_searcher_t :
        public generic_metadata_searcher_t<T,
            typename std::map<uuid_u, deletable_t<T> >,
            typename std::map<uuid_u, deletable_t<T> >::iterator> {
public:
    typedef typename std::map<uuid_u, deletable_t<T> >::iterator iterator;
    typedef typename std::map<uuid_u, deletable_t<T> > metamap_t;
    explicit metadata_searcher_t(metamap_t *_map) :
            generic_metadata_searcher_t<T, metamap_t, iterator>(_map) { }
};
template<class T>
class const_metadata_searcher_t :
        public generic_metadata_searcher_t<T,
            const typename std::map<uuid_u, deletable_t<T> >,
            typename std::map<uuid_u, deletable_t<T> >::const_iterator> {
public:
    typedef typename std::map<uuid_u, deletable_t<T> >::const_iterator iterator;
    typedef const typename std::map<uuid_u, deletable_t<T> > metamap_t;
    explicit const_metadata_searcher_t(metamap_t *_map) :
            generic_metadata_searcher_t<T, metamap_t, iterator>(_map) { }
};

class namespace_predicate_t {
public:
    bool operator()(const namespace_semilattice_metadata_t &ns) const {
        if (name && ns.name.get_ref() != *name) {
            return false;
        } else if (db_id && ns.database.get_ref() != *db_id) {
            return false;
        }
        return true;
    }
    explicit namespace_predicate_t(const name_string_t *_name): name(_name), db_id(NULL) { }
    explicit namespace_predicate_t(const uuid_u *_db_id): name(NULL), db_id(_db_id) { }
    namespace_predicate_t(const name_string_t *_name, const uuid_u *_db_id):
        name(_name), db_id(_db_id) { }
private:
    const name_string_t *name;
    const uuid_u *db_id;

    DISABLE_COPYING(namespace_predicate_t);
};


#endif  // CLUSTERING_ADMINISTRATION_METADATA_HPP_
