#ifndef __CLUSTERING_BACKFILL_METADATA_HPP__
#define __CLUSTERING_BACKFILL_METADATA_HPP__

template<class protocol_t>
struct backfiller_metadata_t {

    typedef boost::uuids::uuid backfill_session_id_t;

    typedef async_mailbox_t<void(
        backfill_session_id_t,
        typename protocol_t::backfill_request_t,
        async_mailbox_t<void(typename protocol_t::backfill_chunk_t)>::address_t,
        async_mailbox_t<void()>::address_t
        )> backfill_mailbox_t;
    backfill_mailbox_t::address_t backfill_mailbox;

    typedef async_mailbox_t<void(
        backfill_session_id_t
        )> cancel_backfill_mailbox_t;
    cancel_backfill_mailbox_t::address_t cancel_backfill_mailbox;
};

#endif /* __CLUSTERING_BACKFILL_METADATA_HPP__ */
