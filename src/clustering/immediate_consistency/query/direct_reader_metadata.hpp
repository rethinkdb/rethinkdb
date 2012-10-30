// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_DIRECT_READER_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_DIRECT_READER_METADATA_HPP_

#include "rpc/mailbox/typed.hpp"

/* Each replica exposes a `direct_reader_business_card_t` for each shard that it
is a primary or secondary for. */

template <class protocol_t>
class direct_reader_business_card_t {
public:
    typedef mailbox_t< void(
            typename protocol_t::read_t,
            mailbox_addr_t< void(typename protocol_t::read_response_t)>
            )> read_mailbox_t;

    direct_reader_business_card_t() { }
    explicit direct_reader_business_card_t(const typename read_mailbox_t::address_t &rm) : read_mailbox(rm) { }

    typename read_mailbox_t::address_t read_mailbox;

    RDB_MAKE_ME_SERIALIZABLE_1(read_mailbox);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_DIRECT_READER_METADATA_HPP_ */
