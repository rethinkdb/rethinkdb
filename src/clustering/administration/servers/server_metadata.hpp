// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_SERVER_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_SERVER_METADATA_HPP_

#include <map>
#include <set>
#include <string>

#include "containers/name_string.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/versioned.hpp"
#include "rpc/serialize_macros.hpp"
#include "utils.hpp"

class server_config_t {
public:
    name_string_t name;
    std::set<name_string_t> tags;
    boost::optional<uint64_t> cache_size_bytes;
};

RDB_DECLARE_SERIALIZABLE(server_config_t);
RDB_DECLARE_EQUALITY_COMPARABLE(server_config_t);

typedef uint64_t server_config_version_t;

class server_config_versioned_t {
public:
    server_config_t config;
    server_config_version_t version;
};

RDB_DECLARE_SERIALIZABLE(server_config_versioned_t);
RDB_DECLARE_EQUALITY_COMPARABLE(server_config_versioned_t);

class server_name_map_t {
public:
    name_string_t get(const server_id_t &sid) const;
    std::map<server_id_t, std::pair<server_config_version_t, name_string_t> > names;
};

RDB_DECLARE_SERIALIZABLE(server_name_map_t);
RDB_DECLARE_EQUALITY_COMPARABLE(server_name_map_t);

class server_config_business_card_t {
public:
    /* On success, the reply will be an empty string and the version of the new change.
    On failure, the reply will be zero and an error string. */
    typedef mailbox_t< void(
            server_config_t,
            mailbox_t<void(server_config_version_t, std::string)>::address_t
        ) > set_config_mailbox_t;
    set_config_mailbox_t::address_t set_config_addr;
};

RDB_DECLARE_SERIALIZABLE(server_config_business_card_t);

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_SERVER_METADATA_HPP_ */
