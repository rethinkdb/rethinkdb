#ifndef __CLUSTERING_BACKFILL_METADATA_HPP__
#define __CLUSTERING_BACKFILL_METADATA_HPP__

#include "errors.hpp"
#include <boost/uuid/uuid.hpp>

#include "clustering/version.hpp"
#include "protocol_api.hpp"
#include "rpc/mailbox/typed.hpp"
#include "timestamps.hpp"

template<class protocol_t>
struct backfiller_metadata_t {

    typedef boost::uuids::uuid backfill_session_id_t;

    typedef async_mailbox_t<void(
        backfill_session_id_t,
        region_map_t<protocol_t, version_range_t>,
        typename async_mailbox_t<void(region_map_t<protocol_t, version_range_t>)>::address_t,
        typename async_mailbox_t<void(typename protocol_t::backfill_chunk_t)>::address_t,
        async_mailbox_t<void()>::address_t
        )> backfill_mailbox_t;

    typedef async_mailbox_t<void(
        backfill_session_id_t
        )> cancel_backfill_mailbox_t;

    backfiller_metadata_t() { }
    backfiller_metadata_t(
            const typename backfill_mailbox_t::address_t &ba,
            const cancel_backfill_mailbox_t::address_t &cba) :
        backfill_mailbox(ba), cancel_backfill_mailbox(cba)
        { }

    typename backfill_mailbox_t::address_t backfill_mailbox;
    cancel_backfill_mailbox_t::address_t cancel_backfill_mailbox;
};

template<class protocol_t>
void semilattice_join(UNUSED backfiller_metadata_t<protocol_t> *a, UNUSED const backfiller_metadata_t<protocol_t> &b) {
    /* They should be equal, but we don't bother testing that */
}

#endif /* __CLUSTERING_BACKFILL_METADATA_HPP__ */
