#ifndef __CLUSTERING_IMMEDIATE_CONSISTENCY_MASTER_METADATA_HPP__
#define __CLUSTERING_IMMEDIATE_CONSISTENCY_MASTER_METADATA_HPP__

#include "errors.hpp"
#include <boost/variant.hpp>

#include "concurrency/fifo_checker.hpp"
#include "rpc/mailbox/typed.hpp"

typedef boost::uuids::uuid master_id_t;

/* There is one `master_metadata_t` per branch. It's created by the master.
Parsers use it to find the master. */

template<class protocol_t>
class master_metadata_t {

public:
    /* Mailbox types for the master */
    typedef async_mailbox_t<void(
        typename protocol_t::read_t,
        order_token_t,
        typename async_mailbox_t<void(boost::variant<
            typename protocol_t::read_response_t,
            std::string
            >)>::address_t
        )> read_mailbox_t;
    typedef async_mailbox_t<void(
        typename protocol_t::write_t,
        order_token_t,
        typename async_mailbox_t<void(boost::variant<
            typename protocol_t::write_response_t,
            std::string
            >)>::address_t
        )> write_mailbox_t;

    master_metadata_t() { }
    master_metadata_t(
            const typename read_mailbox_t::address_t rm,
            const typename write_mailbox_t::address_t wm) :
        read_mailbox(rm), write_mailbox(wm) { }

    /* The region that this master covers */
    typename protocol_t::region_t region;

    /* Contact info for the master itself */
    typename read_mailbox_t::address_t read_mailbox;
    typename write_mailbox_t::address_t write_mailbox;

    RDB_MAKE_ME_SERIALIZABLE_2(read_mailbox, write_mailbox);
};

template<class protocol_t>
void semilattice_join(UNUSED master_metadata_t<protocol_t> *a, UNUSED const master_metadata_t<protocol_t> &b) {
    /* The mailboxes should be identical, but we don't check. */
}

#endif /* __CLUSTERING_IMMEDIATE_CONSISTENCY_MASTER_METADATA_HPP__ */
