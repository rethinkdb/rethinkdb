#ifndef __CLUSTERING_BACKFILL_METADATA_HPP__
#define __CLUSTERING_BACKFILL_METADATA_HPP__

#include "errors.hpp"
#include <boost/uuid/uuid.hpp>

#include "rpc/mailbox/typed.hpp"
#include "timestamps.hpp"

template<class protocol_t>
struct backfiller_metadata_t {

    typedef boost::uuids::uuid backfill_session_id_t;

    typedef async_mailbox_t<void(
        backfill_session_id_t,
        typename protocol_t::store_t::backfill_request_t,
        typename async_mailbox_t<void(typename protocol_t::store_t::backfill_chunk_t)>::address_t,
        typename async_mailbox_t<void(state_timestamp_t)>::address_t
        )> backfill_mailbox_t;
    typename backfill_mailbox_t::address_t backfill_mailbox;

    typedef async_mailbox_t<void(
        backfill_session_id_t
        )> cancel_backfill_mailbox_t;
    cancel_backfill_mailbox_t::address_t cancel_backfill_mailbox;
};

template<class protocol_t>
void semilattice_join(UNUSED backfiller_metadata_t<protocol_t> *a, UNUSED const backfiller_metadata_t<protocol_t> &b) {
    /* They should be equal, but we don't bother testing that */
}

#endif /* __CLUSTERING_BACKFILL_METADATA_HPP__ */
