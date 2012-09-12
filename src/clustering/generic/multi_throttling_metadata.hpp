#ifndef CLUSTERING_GENERIC_MULTI_THROTTLING_METADATA_HPP_
#define CLUSTERING_GENERIC_MULTI_THROTTLING_METADATA_HPP_

#include "clustering/generic/registration_metadata.hpp"

template <class request_type, class inner_client_business_card_type>
class multi_throttling_business_card_t {
public:
    class server_business_card_t {
    public:
        server_business_card_t() { }
        server_business_card_t(
                const mailbox_addr_t<void(request_type)> &ra,
                const mailbox_addr_t<void(int)> &rta) :
            request_addr(ra), relinquish_tickets_addr(rta) { }
        mailbox_addr_t<void(request_type)> request_addr;
        mailbox_addr_t<void(int)> relinquish_tickets_addr;
        RDB_MAKE_ME_SERIALIZABLE_2(request_addr, relinquish_tickets_addr);
    };

    class client_business_card_t {
    public:
        client_business_card_t() { }
        client_business_card_t(
                const inner_client_business_card_type &icbc,
                const mailbox_addr_t<void(server_business_card_t)> &ia,
                const mailbox_addr_t<void(int)> &gta,
                const mailbox_addr_t<void(int)> &rta) :
            inner_client_business_card(icbc), intro_addr(ia),
            give_tickets_addr(gta), reclaim_tickets_addr(rta) { }
        inner_client_business_card_type inner_client_business_card;
        mailbox_addr_t<void(server_business_card_t)> intro_addr;
        mailbox_addr_t<void(int)> give_tickets_addr;
        mailbox_addr_t<void(int)> reclaim_tickets_addr;
        RDB_MAKE_ME_SERIALIZABLE_4(inner_client_business_card,
            intro_addr, give_tickets_addr, reclaim_tickets_addr);
    };

    multi_throttling_business_card_t() { }
    explicit multi_throttling_business_card_t(
            const registrar_business_card_t<client_business_card_t> &r) :
        registrar(r) { }
    registrar_business_card_t<client_business_card_t> registrar;
    RDB_MAKE_ME_SERIALIZABLE_1(registrar);
};

#endif /* CLUSTERING_GENERIC_MULTI_THROTTLING_METADATA_HPP_ */
