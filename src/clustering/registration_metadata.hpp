#ifndef __CLUSTERING_REGISTRATION_HPP__
#define __CLUSTERING_REGISTRATION_HPP__

#include "rpc/mailbox/typed.hpp"
#include <boost/uuid/uuid.hpp>

template<class data_t>
class registrar_metadata_t {

public:
    typedef boost::uuids::uuid registration_id_t;

    typedef async_mailbox_t<void(
        registration_id_t,
        peer_id_t,
        data_t
        )> create_mailbox_t;
    typename create_mailbox_t::address_t create_mailbox;

    typedef async_mailbox_t<void(
        registration_id_t
        )> delete_mailbox_t;
    typename delete_mailbox_t::address_t delete_mailbox;

    registrar_metadata_t() { }

    registrar_metadata_t(
            const typename create_mailbox_t::address_t &cm,
            const typename delete_mailbox_t::address_t &dm) :
        create_mailbox(cm), delete_mailbox(dm)
        { }
};

template<class data_t>
void semilattice_join(UNUSED registrar_metadata_t<data_t> *a, UNUSED const registrar_metadata_t<data_t> &b) {
    /* They should be identical. Do nothing. */
}

#endif /* __CLUSTERING_REGISTRATION_HPP__ */
