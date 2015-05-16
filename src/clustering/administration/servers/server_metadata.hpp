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
    boost::optional<uint64_t> cache_size;   /* in bytes */
};

RDB_DECLARE_SERIALIZABLE(server_config_t);

class server_config_business_card_t {
public:
    /* An empty reply string means success */
    typedef mailbox_t< void(
            server_config_t_t,
            mailbox_t<void(std::string)>::address_t
        ) > set_config_mailbox_t;
    set_config_mailbox_t::address_t set_config_addr;
};

RDB_DECLARE_SERIALIZABLE(server_config_business_card_t);

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_SERVER_METADATA_HPP_ */
