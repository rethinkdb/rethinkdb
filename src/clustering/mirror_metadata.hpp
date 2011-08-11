#ifndef __CLUSTERING_MIRROR_HPP__
#define __CLUSTERING_MIRROR_HPP__

/* Each up-to-date mirror has an entry in the metadata. The purpose of these
entries is solely so that other nodes can find mirrors to request backfills from
them or to send read-outdated queries; the masters find the mirrors via a
different mechanism. */

template<class protocol_t>
struct mirror_metadata_t {

    /* If `alive` is `true`, the mirror is active and can provide backfills or
    service read-outdated queries. If `alive` is `false`, the mirror is no
    longer active and the mailboxes will be nil. */

    bool alive;

    /* This mailbox services read-outdated requests. */

    typedef async_mailbox_t<void(
        typename protocol_t::read_t,
        async_mailbox_t<void(typename protocol_t::read_response_t)>::address_t
        )> read_mailbox_t;
    read_mailbox_t::address_t read_mailbox;

    /* This is the mailbox that new mirrors can send a backfill-request to. */

    typedef async_mailbox_t<void(
        typename protocol_t::backfill_request_t,
        async_mailbox_t<void(typename protocol_t::backfill_chunk_t)>::address_t,
        async_mailbox_t<void()>::address_t
        )> backfill_mailbox_t;
    backfill_mailbox_t::address_t backfill_mailbox;
};

typedef boost::uuids::uuid mirror_id_t;

#endif /* __CLUSTERING_MIRROR_HPP__ */