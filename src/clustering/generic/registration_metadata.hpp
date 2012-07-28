#ifndef CLUSTERING_GENERIC_REGISTRATION_METADATA_HPP_
#define CLUSTERING_GENERIC_REGISTRATION_METADATA_HPP_

#include "containers/uuid.hpp"
#include "rpc/mailbox/typed.hpp"

template<class business_card_t>
class registrar_business_card_t {

public:
    typedef uuid_t registration_id_t;

    typedef mailbox_t<void(registration_id_t, peer_id_t, business_card_t)> create_mailbox_t;
    typename create_mailbox_t::address_t create_mailbox;

    typedef mailbox_t<void(registration_id_t)> delete_mailbox_t;
    typename delete_mailbox_t::address_t delete_mailbox;

    registrar_business_card_t() { }

    registrar_business_card_t(
            const typename create_mailbox_t::address_t &cm,
            const typename delete_mailbox_t::address_t &dm) :
        create_mailbox(cm), delete_mailbox(dm)
        { }

    RDB_MAKE_ME_SERIALIZABLE_2(create_mailbox, delete_mailbox);
};

#endif /* CLUSTERING_GENERIC_REGISTRATION_METADATA_HPP_ */
