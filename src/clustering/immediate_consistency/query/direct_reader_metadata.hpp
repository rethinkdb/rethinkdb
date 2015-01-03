// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_DIRECT_READER_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_DIRECT_READER_METADATA_HPP_

#include "rdb_protocol/protocol.hpp"
#include "rpc/mailbox/typed.hpp"

/* Each replica exposes a `direct_reader_business_card_t` for each shard that it
is a primary or secondary replica for. */

class direct_reader_business_card_t {
public:
    typedef mailbox_t< void(
            read_t,
            mailbox_addr_t< void(read_response_t)>
            )> read_mailbox_t;

    direct_reader_business_card_t() { }
    explicit direct_reader_business_card_t(const read_mailbox_t::address_t &rm) : read_mailbox(rm) { }

    read_mailbox_t::address_t read_mailbox;
};

RDB_DECLARE_SERIALIZABLE(direct_reader_business_card_t);
RDB_DECLARE_EQUALITY_COMPARABLE(direct_reader_business_card_t);

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_DIRECT_READER_METADATA_HPP_ */
