// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_SERVER_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_SERVER_METADATA_HPP_

#include <map>
#include <set>
#include <string>

#include "containers/name_string.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/versioned.hpp"
#include "rpc/serialize_macros.hpp"
#include "utils.hpp"

class server_semilattice_metadata_t {
public:
    /* These fields should only be modified by the server that this metadata is
    describing. */

    versioned_t<name_string_t> name;
    versioned_t<std::set<name_string_t> > tags;

    /* An empty `boost::optional` means to pick a reasonable cache size automatically. */
    versioned_t<boost::optional<uint64_t> > cache_size_bytes;
};

RDB_DECLARE_SERIALIZABLE(server_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(server_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(server_semilattice_metadata_t);

class servers_semilattice_metadata_t {
public:
    typedef std::map<server_id_t, deletable_t<server_semilattice_metadata_t> > server_map_t;
    server_map_t servers;
};

RDB_DECLARE_SERIALIZABLE(servers_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(servers_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(servers_semilattice_metadata_t);

class server_config_business_card_t {
public:
    /* In all cases, an empty reply string means success */

    typedef mailbox_t< void(
            name_string_t,
            mailbox_t<void(std::string)>::address_t
        ) > change_name_mailbox_t;
    change_name_mailbox_t::address_t change_name_addr;

    typedef mailbox_t< void(
            std::set<name_string_t>,
            mailbox_t<void(std::string)>::address_t
        ) > change_tags_mailbox_t;
    change_tags_mailbox_t::address_t change_tags_addr;

    typedef mailbox_t< void(
            boost::optional<uint64_t>,   /* in bytes */
            mailbox_t<void(std::string)>::address_t
        ) > change_cache_size_mailbox_t;
    change_cache_size_mailbox_t::address_t change_cache_size_addr;
};

RDB_DECLARE_SERIALIZABLE(server_config_business_card_t);

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_SERVER_METADATA_HPP_ */
