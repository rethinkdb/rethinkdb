#ifndef __CLUSTERING_REGISTRATION_HPP__
#define __CLUSTERING_REGISTRATION_HPP__

#include "concurrency/fifo_checker.hpp"
#include "rpc/mailbox/typed.hpp"
#include <boost/uuids/uuid.hpp>

template<class protocol_t>
struct registration_metadata_t {

    typedef async_mailbox_t<void(
        typename protocol_t::write_t,
        repli_timestamp_t, order_token_t,
        async_mailbox_t<void()>::address_t
        )> write_mailbox_t;

    typedef async_mailbox_t<void(
        typename protocol_t::write_t,
        repli_timestamp_t, order_token_t,
        async_mailbox_t<void(typename protocol_t::write_response_t)>
        )> writeread_mailbox_t;

    typedef async_mailbox_t<void(
        typename protocol_t::read_t,
        order_token_t,
        async_mailbox_t<void(typename protocol_t::read_response_t)>
        )> read_mailbox_t;

    typedef boost::uuids::uuid registration_id_t;

    typedef async_mailbox_t<void(
        registration_id_t,
        write_mailbox_t::address_t
        )> register_mailbox_t;
    register_mailbox_t::address_t register_mailbox;

    typedef async_mailbox_t<void(
        registration_id_t,
        writeread_mailbox_t::address_t,
        read_mailbox_t::address_t
        )> upgrade_mailbox_t;
    upgrade_mailbox_t::address_t upgrade_mailbox;

    typedef async_mailbox_t<void(
        registration_id_t
        )> deregister_mailbox_t:
    deregister_mailbox_t::address_t deregister_mailbox;
};

#endif /* __CLUSTERING_REGISTRATION_HPP__ */
