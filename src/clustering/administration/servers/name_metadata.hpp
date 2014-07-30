// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_NAME_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_NAME_METADATA_HPP_

#include "containers/name_string.hpp"
#include "rpc/mailbox/typed.hpp"

class server_name_business_card_t {
public:
    /* The address to send name-change orders. */
    typedef mailbox_t< void(
            name_string_t,
            mailbox_t<void()>::address_t
        ) > rename_mailbox_t;
    rename_mailbox_t::address_t rename_addr;

    /* The time when the server started up. Used for resolving name collisions; the
    server that's been alive longer gets to keep the name. */
    time_t startup_time;
};

RDB_DECLARE_SERIALIZABLE(server_name_business_card_t);

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_NAME_METADATA_HPP_ */

