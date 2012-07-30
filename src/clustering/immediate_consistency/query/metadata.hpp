#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_METADATA_HPP_

#include <string>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "clustering/generic/registration_metadata.hpp"
#include "clustering/generic/resource.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "rpc/mailbox/typed.hpp"

typedef uuid_t master_access_id_t;

struct master_access_business_card_t {
    typedef mailbox_t<void()> ack_mailbox_type;
    typedef mailbox_t<void(int)> allocation_mailbox_t;

    master_access_business_card_t(master_access_id_t maid, const ack_mailbox_type::address_t &_ack_address,
                                        const allocation_mailbox_t::address_t &_allocation_address)
        : master_access_id(maid), ack_address(_ack_address), allocation_address(_allocation_address) 
    { }
    master_access_business_card_t() { }

    master_access_id_t master_access_id;
    ack_mailbox_type::address_t ack_address;
    allocation_mailbox_t::address_t allocation_address;
    RDB_MAKE_ME_SERIALIZABLE_3(master_access_id, ack_address, allocation_address);
};

/* There is one `master_business_card_t` per branch. It's created by the master.
Parsers use it to find the master. */

template<class protocol_t>
class master_business_card_t {

public:
    /* Mailbox types for the master */
    typedef mailbox_t< void(
        master_access_id_t,
        typename protocol_t::read_t,
        order_token_t,
        fifo_enforcer_read_token_t,
        mailbox_addr_t< void(boost::variant<typename protocol_t::read_response_t, std::string>)>
        )> read_mailbox_t;
    typedef mailbox_t< void(
        master_access_id_t,
        typename protocol_t::write_t,
        order_token_t,
        fifo_enforcer_write_token_t,
        mailbox_addr_t< void(boost::variant<typename protocol_t::write_response_t, std::string>)>
        )> write_mailbox_t;

    master_business_card_t() { }
    master_business_card_t(const typename protocol_t::region_t &r,
                           const typename read_mailbox_t::address_t &rm,
                           const typename write_mailbox_t::address_t &wm,
                           const registrar_business_card_t<master_access_business_card_t> &marbc)
        : region(r), read_mailbox(rm), write_mailbox(wm), master_access_registration_business_card(marbc) { }

    /* The region that this master covers */
    typename protocol_t::region_t region;

    /* Contact info for the master itself */
    typename read_mailbox_t::address_t read_mailbox;
    typename write_mailbox_t::address_t write_mailbox;

    registrar_business_card_t<master_access_business_card_t> master_access_registration_business_card;

    RDB_MAKE_ME_SERIALIZABLE_4(region, read_mailbox, write_mailbox, master_access_registration_business_card);
};

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

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_METADATA_HPP_ */
