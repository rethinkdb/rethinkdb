// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_MULTI_THROTTLING_METADATA_HPP_
#define CLUSTERING_GENERIC_MULTI_THROTTLING_METADATA_HPP_

#include "clustering/generic/registration_metadata.hpp"
#include "rpc/semilattice/joins/macros.hpp"

template <class request_t>
class multi_throttling_server_business_card_t {
public:
    multi_throttling_server_business_card_t() { }
    multi_throttling_server_business_card_t(
            const mailbox_addr_t<void(request_t)> &ra,
            const mailbox_addr_t<void(int)> &rta) :
        request_addr(ra), relinquish_tickets_addr(rta) { }
    mailbox_addr_t<void(request_t)> request_addr;
    mailbox_addr_t<void(int)> relinquish_tickets_addr;
    RDB_MAKE_ME_SERIALIZABLE_2(multi_throttling_server_business_card_t,
        request_addr, relinquish_tickets_addr);
    RDB_MAKE_ME_EQUALITY_COMPARABLE_2(multi_throttling_server_business_card_t,
                                      request_addr, relinquish_tickets_addr);
};

template <class request_t, class inner_client_business_card_t>
class multi_throttling_client_business_card_t {
public:
    multi_throttling_client_business_card_t() { }
    multi_throttling_client_business_card_t(
            const inner_client_business_card_t &icbc,
            const mailbox_addr_t<void(multi_throttling_server_business_card_t<request_t>)> &ia,
            const mailbox_addr_t<void(int)> &gta,
            const mailbox_addr_t<void(int)> &rta) :
        inner_client_business_card(icbc), intro_addr(ia),
        give_tickets_addr(gta), reclaim_tickets_addr(rta) { }
    inner_client_business_card_t inner_client_business_card;
    mailbox_addr_t<void(multi_throttling_server_business_card_t<request_t>)> intro_addr;
    mailbox_addr_t<void(int)> give_tickets_addr;
    mailbox_addr_t<void(int)> reclaim_tickets_addr;
    RDB_MAKE_ME_SERIALIZABLE_4(multi_throttling_client_business_card_t,
        inner_client_business_card, intro_addr, give_tickets_addr, reclaim_tickets_addr);
    RDB_MAKE_ME_EQUALITY_COMPARABLE_4(
            multi_throttling_client_business_card_t,
            inner_client_business_card, intro_addr, give_tickets_addr,
            reclaim_tickets_addr);
};

template <class request_t, class inner_client_business_card_t>
class multi_throttling_business_card_t {
public:
    multi_throttling_business_card_t() { }
    typedef multi_throttling_server_business_card_t<request_t> server_business_card_t;
    typedef multi_throttling_client_business_card_t<request_t, inner_client_business_card_t>
        client_business_card_t;

    explicit multi_throttling_business_card_t(
            const registrar_business_card_t<client_business_card_t> &r) :
        registrar(r) { }
    registrar_business_card_t<client_business_card_t> registrar;
    RDB_MAKE_ME_SERIALIZABLE_1(multi_throttling_business_card_t, registrar);

    bool operator==(const multi_throttling_business_card_t &other) const {
        return registrar == other.registrar;
    }
};

#endif /* CLUSTERING_GENERIC_MULTI_THROTTLING_METADATA_HPP_ */
