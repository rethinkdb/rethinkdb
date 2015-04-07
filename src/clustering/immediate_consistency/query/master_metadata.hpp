// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_METADATA_HPP_

#include <string>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "clustering/generic/multi_client_metadata.hpp"
#include "clustering/generic/registration_metadata.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/mailbox/typed.hpp"

/* There is one `master_business_card_t` per branch. It's created by the master.
Parsers use it to find the master. */

class master_business_card_t {
public:
    class read_request_t {
    public:
        read_request_t() { }
        read_request_t(
                const read_t &r,
                order_token_t ot,
                fifo_enforcer_read_token_t ft,
                const mailbox_addr_t< void(boost::variant<read_response_t, std::string>)> &ca) :
            read(r), order_token(ot), fifo_token(ft), cont_addr(ca) { }
        read_t read;
        order_token_t order_token;
        fifo_enforcer_read_token_t fifo_token;
        mailbox_addr_t< void(boost::variant<read_response_t, std::string>)> cont_addr;
    };

    class write_request_t {
    public:
        write_request_t() { }
        write_request_t(
                const write_t &w,
                order_token_t ot,
                fifo_enforcer_write_token_t ft,
                const mailbox_addr_t< void(boost::variant<write_response_t, std::string>)> &ca) :
            write(w), order_token(ot), fifo_token(ft), cont_addr(ca) { }
        write_t write;
        order_token_t order_token;
        fifo_enforcer_write_token_t fifo_token;
        mailbox_addr_t< void(boost::variant<write_response_t, std::string>)> cont_addr;
    };

    typedef boost::variant< read_request_t, write_request_t > request_t;

    class inner_client_business_card_t {
    public:
        /* nothing here */
    };

    master_business_card_t() { }
    master_business_card_t(const region_t &r,
                           const multi_client_business_card_t<
                               request_t,
                               inner_client_business_card_t> &mc)
        : region(r), multi_client(mc) { }

    /* The region that this master covers */
    region_t region;

    /* Contact info for the master itself */
    multi_client_business_card_t<request_t, inner_client_business_card_t> multi_client;
};

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(master_business_card_t::read_request_t);

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(master_business_card_t::write_request_t);

RDB_DECLARE_SERIALIZABLE(master_business_card_t::inner_client_business_card_t);

RDB_DECLARE_SERIALIZABLE(master_business_card_t);
RDB_DECLARE_EQUALITY_COMPARABLE(master_business_card_t);

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_METADATA_HPP_ */
