// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_MULTI_CLIENT_METADATA_HPP_
#define CLUSTERING_GENERIC_MULTI_CLIENT_METADATA_HPP_

#include "clustering/generic/registration_metadata.hpp"
#include "rpc/semilattice/joins/macros.hpp"

template <class request_t>
class multi_client_client_business_card_t {
public:
    multi_client_client_business_card_t() { }
    multi_client_client_business_card_t(
            const mailbox_addr_t<void(mailbox_addr_t<void(request_t)>)> &ia) :
        intro_addr(ia) { }
    mailbox_addr_t<void(mailbox_addr_t<void(request_t)>)> intro_addr;
    RDB_MAKE_ME_SERIALIZABLE_1(multi_client_client_business_card_t, intro_addr);
    RDB_MAKE_ME_EQUALITY_COMPARABLE_1(multi_client_client_business_card_t, intro_addr);
};

template <class request_t>
class multi_client_business_card_t {
public:
    multi_client_business_card_t() { }
    typedef mailbox_addr_t<void(request_t)> server_business_card_t;
    typedef multi_client_client_business_card_t<request_t>
        client_business_card_t;

    explicit multi_client_business_card_t(
            const registrar_business_card_t<client_business_card_t> &r) :
        registrar(r) { }
    registrar_business_card_t<client_business_card_t> registrar;
    RDB_MAKE_ME_SERIALIZABLE_1(multi_client_business_card_t, registrar);
    RDB_MAKE_ME_EQUALITY_COMPARABLE_1(multi_client_business_card_t, registrar);
};

#endif /* CLUSTERING_GENERIC_MULTI_CLIENT_METADATA_HPP_ */
