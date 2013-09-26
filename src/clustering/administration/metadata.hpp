// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_METADATA_HPP_

#include <list>
#include <map>
#include <string>
#include <vector>
#include <utility>

// TODO: Probably some of these headers could be moved to the .cc.
#include "clustering/administration/datacenter_metadata.hpp"
#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/log_transfer.hpp"
#include "clustering/administration/machine_metadata.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "clustering/administration/stat_manager.hpp"
#include "clustering/administration/metadata_change_handler.hpp"
#include "containers/archive/cow_ptr_type.hpp"
#include "containers/cow_ptr.hpp"
#include "containers/auth_key.hpp"
#include "http/json/json_adapter.hpp"
#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/semilattice/joins/cow_ptr.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/serialize_macros.hpp"

namespace mock { class dummy_protocol_t; }
class memcached_protocol_t;
struct rdb_protocol_t;

class cluster_semilattice_metadata_t {
public:
    cluster_semilattice_metadata_t() { }

    cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> > dummy_namespaces;
    cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> > memcached_namespaces;
    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > rdb_namespaces;

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

class auth_semilattice_metadata_t {
public:
    auth_semilattice_metadata_t() { }

    vclock_t<auth_key_t> auth_key;

    RDB_MAKE_ME_SERIALIZABLE_1(auth_key);
};

RDB_MAKE_SEMILATTICE_JOINABLE_1(auth_semilattice_metadata_t, auth_key);
RDB_MAKE_EQUALITY_COMPARABLE_1(auth_semilattice_metadata_t, auth_key);

// json adapter concept for auth_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(auth_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
cJSON *with_ctx_render_as_json(auth_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_apply_json_to(cJSON *change, auth_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_on_subfield_change(auth_semilattice_metadata_t *, const vclock_ctx_t &);

enum cluster_directory_peer_type_t {
    ADMIN_PEER,
    SERVER_PEER,
    PROXY_PEER
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(cluster_directory_peer_type_t, int8_t, ADMIN_PEER, PROXY_PEER);

class cluster_directory_metadata_t {
public:

    cluster_directory_metadata_t() { }
    cluster_directory_metadata_t(
            machine_id_t mid,
            peer_id_t pid,
            const std::vector<std::string> &_ips,
            const get_stats_mailbox_address_t& _stats_mailbox,
            const metadata_change_handler_t<cluster_semilattice_metadata_t>::request_mailbox_t::address_t& _semilattice_change_mailbox,
            const metadata_change_handler_t<auth_semilattice_metadata_t>::request_mailbox_t::address_t& _auth_change_mailbox,
            const log_server_business_card_t &lmb,
            cluster_directory_peer_type_t _peer_type) :
        machine_id(mid),
        peer_id(pid),
        ips(_ips),
        get_stats_mailbox_address(_stats_mailbox),
        semilattice_change_mailbox(_semilattice_change_mailbox),
        auth_change_mailbox(_auth_change_mailbox),
        log_mailbox(lmb),
        peer_type(_peer_type) { }

    namespaces_directory_metadata_t<mock::dummy_protocol_t> dummy_namespaces;
    namespaces_directory_metadata_t<memcached_protocol_t> memcached_namespaces;
    namespaces_directory_metadata_t<rdb_protocol_t> rdb_namespaces;

    /* Tell the other peers what our machine ID is */
    machine_id_t machine_id;
    peer_id_t peer_id;

    /* To tell everyone what our ips are. */
    std::vector<std::string> ips;

    get_stats_mailbox_address_t get_stats_mailbox_address;
    metadata_change_handler_t<cluster_semilattice_metadata_t>::request_mailbox_t::address_t semilattice_change_mailbox;
    metadata_change_handler_t<auth_semilattice_metadata_t>::request_mailbox_t::address_t auth_change_mailbox;
    log_server_business_card_t log_mailbox;
    std::list<local_issue_t> local_issues;
    cluster_directory_peer_type_t peer_type;

    RDB_MAKE_ME_SERIALIZABLE_12(dummy_namespaces, memcached_namespaces, rdb_namespaces, machine_id, peer_id, ips, get_stats_mailbox_address, semilattice_change_mailbox, auth_change_mailbox, log_mailbox, local_issues, peer_type);
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

// ctx-less json adapter concept for cluster_directory_metadata_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(cluster_directory_metadata_t *target);
cJSON *render_as_json(cluster_directory_metadata_t *target);
void apply_json_to(cJSON *change, cluster_directory_metadata_t *target);


// ctx-less json adapter for cluster_directory_peer_type_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(cluster_directory_peer_type_t *);
cJSON *render_as_json(cluster_directory_peer_type_t *peer_type);
void apply_json_to(cJSON *, cluster_directory_peer_type_t *);

enum metadata_search_status_t {
    METADATA_SUCCESS, METADATA_ERR_NONE, METADATA_ERR_MULTIPLE, METADATA_CONFLICT
};

/* A helper class to search through metadata in various ways.  Can be
   constructed from a pointer to the internal map of the metadata,
   e.g. `metadata.databases.databases`.  Look in rdb_protocol/query_language.cc
   for examples on how to use. */
template<class T>
class metadata_searcher_t {
public:
    typedef std::map<uuid_u, deletable_t<T> > metamap_t;
    typedef typename metamap_t::iterator iterator;
    iterator begin() {return map->begin();}
    iterator end() {return map->end();}

    explicit metadata_searcher_t(metamap_t *_map): map(_map) { }

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
        for (it = start; it != end(); ++it) if (!it->second.is_deleted()) break;
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
            return !metadata.name.in_conflict() && metadata.name.get() == *name;
        }
        explicit name_predicate_t(const name_string_t *_name): name(_name) { }
    private:
        const name_string_t *name;
    };
private:
    metamap_t *map;
};

class namespace_predicate_t {
public:
    bool operator()(const namespace_semilattice_metadata_t<rdb_protocol_t>& ns) const {
        if (name && (ns.name.in_conflict() || ns.name.get() != *name)) {
            return false;
        } else if (db_id && (ns.database.in_conflict() || ns.database.get() != *db_id)) {
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
