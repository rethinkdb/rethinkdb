// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_QUERY_ROUTING_METADATA_HPP_
#define CLUSTERING_QUERY_ROUTING_METADATA_HPP_

#include <string>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "clustering/generic/multi_client_metadata.hpp"
#include "clustering/generic/registration_metadata.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/mailbox/typed.hpp"

/* There is one `primary_query_bcard_t` per branch. It's created by the primary; parsers
use it to find the master. */

class primary_query_bcard_t {
public:
    class read_request_t {
    public:
        read_request_t() { }
        read_request_t(
                const read_t &r,
                order_token_t ot,
                fifo_enforcer_read_token_t ft,
                const mailbox_addr_t< void(boost::variant<read_response_t, cannot_perform_query_exc_t>)> &ca) :
            read(r), order_token(ot), fifo_token(ft), cont_addr(ca) { }
        read_t read;
        order_token_t order_token;
        fifo_enforcer_read_token_t fifo_token;
        mailbox_addr_t< void(boost::variant<read_response_t, cannot_perform_query_exc_t>)> cont_addr;
    };

    class write_request_t {
    public:
        write_request_t() { }
        write_request_t(
                const write_t &w,
                order_token_t ot,
                fifo_enforcer_write_token_t ft,
                const mailbox_addr_t< void(boost::variant<write_response_t, cannot_perform_query_exc_t>)> &ca) :
            write(w), order_token(ot), fifo_token(ft), cont_addr(ca) { }
        write_t write;
        order_token_t order_token;
        fifo_enforcer_write_token_t fifo_token;
        mailbox_addr_t< void(boost::variant<write_response_t, cannot_perform_query_exc_t>)> cont_addr;
    };

    typedef boost::variant< read_request_t, write_request_t > request_t;

    primary_query_bcard_t() { }
    primary_query_bcard_t(
            const region_t &r,
            const multi_client_business_card_t<request_t> &mc)
        : region(r), multi_client(mc) { }

    /* The region that this master covers */
    region_t region;

    /* Contact info for the master itself */
    multi_client_business_card_t<request_t> multi_client;
};

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(primary_query_bcard_t::read_request_t);

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(primary_query_bcard_t::write_request_t);

RDB_DECLARE_SERIALIZABLE(primary_query_bcard_t);
RDB_DECLARE_EQUALITY_COMPARABLE(primary_query_bcard_t);

/* Each replica exposes a `direct_query_bcard_t` for each shard that it is a primary or
secondary replica for. */

class direct_query_bcard_t {
public:
    typedef mailbox_t< void(
            read_t,
            mailbox_addr_t< void(read_response_t)>
            )> read_mailbox_t;

    direct_query_bcard_t() { }
    explicit direct_query_bcard_t(const read_mailbox_t::address_t &rm) : read_mailbox(rm) { }

    read_mailbox_t::address_t read_mailbox;
};

RDB_DECLARE_SERIALIZABLE(direct_query_bcard_t);
RDB_DECLARE_EQUALITY_COMPARABLE(direct_query_bcard_t);

/* `table_query_bcard_t` wraps the `primary_query_bcard_t` and/or `direct_query_bcard_t`
into a single object for convenience. */

class table_query_bcard_t {
public:
    region_t region;
    boost::optional<primary_query_bcard_t> primary;
    boost::optional<direct_query_bcard_t> direct;
};

RDB_DECLARE_SERIALIZABLE(table_query_bcard_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_query_bcard_t);

#endif /* CLUSTERING_QUERY_ROUTING_METADATA_HPP_ */

