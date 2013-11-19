// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_METADATA_HPP_

#include <string>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "clustering/generic/multi_throttling_metadata.hpp"
#include "clustering/generic/registration_metadata.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "containers/archive/stl_types.hpp"
#include "rpc/mailbox/typed.hpp"

/* There is one `master_business_card_t` per branch. It's created by the master.
Parsers use it to find the master. */

template<class protocol_t>
class master_business_card_t {

public:
    class read_request_t {
    public:
        read_request_t() { }
        read_request_t(
                const typename protocol_t::read_t &r,
                order_token_t ot,
                fifo_enforcer_read_token_t ft,
                const mailbox_addr_t< void(boost::variant<typename protocol_t::read_response_t, std::string>)> &ca) :
            read(r), order_token(ot), fifo_token(ft), cont_addr(ca) { }
        typename protocol_t::read_t read;
        order_token_t order_token;
        fifo_enforcer_read_token_t fifo_token;
        mailbox_addr_t< void(boost::variant<typename protocol_t::read_response_t, std::string>)> cont_addr;
        RDB_MAKE_ME_SERIALIZABLE_4(read, order_token, fifo_token, cont_addr);
    };

    class write_request_t {
    public:
        write_request_t() { }
        write_request_t(
                const typename protocol_t::write_t &w,
                order_token_t ot,
                fifo_enforcer_write_token_t ft,
                const mailbox_addr_t< void(boost::variant<typename protocol_t::write_response_t, std::string>)> &ca) :
            write(w), order_token(ot), fifo_token(ft), cont_addr(ca) { }
        typename protocol_t::write_t write;
        order_token_t order_token;
        fifo_enforcer_write_token_t fifo_token;
        mailbox_addr_t< void(boost::variant<typename protocol_t::write_response_t, std::string>)> cont_addr;
        RDB_MAKE_ME_SERIALIZABLE_4(write, order_token, fifo_token, cont_addr);
    };

    typedef boost::variant< read_request_t, write_request_t > request_t;

    class inner_client_business_card_t {
    public:
        /* nothing here */
        RDB_MAKE_ME_SERIALIZABLE_0();
    };

    master_business_card_t() { }
    master_business_card_t(const typename protocol_t::region_t &r,
                           const multi_throttling_business_card_t<request_t, inner_client_business_card_t> &mt)
        : region(r), multi_throttling(mt) { }

    /* The region that this master covers */
    typename protocol_t::region_t region;

    /* Contact info for the master itself */
    multi_throttling_business_card_t<request_t, inner_client_business_card_t> multi_throttling;

    RDB_MAKE_ME_SERIALIZABLE_2(region, multi_throttling);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_METADATA_HPP_ */
