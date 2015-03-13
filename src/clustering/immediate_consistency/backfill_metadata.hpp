// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_

/* `backfiller_business_card_t` represents a thing that is willing to serve
backfills over the network. It appears in the directory. */

struct backfiller_business_card_t {

    typedef mailbox_t< void(
        backfill_session_id_t,
        region_map_t<version_t>,
        branch_history_t,
        mailbox_addr_t< void(
            region_map_t<version_t>,
            branch_history_t
            ) >,
        mailbox_addr_t< void(
            backfill_chunk_t,
            double,
            fifo_enforcer_write_token_t
            ) >,
        mailbox_t<void(fifo_enforcer_write_token_t)>::address_t,
        mailbox_t<void(mailbox_addr_t<void(int)>)>::address_t
        )> backfill_mailbox_t;

    typedef mailbox_t<void(backfill_session_id_t)> cancel_backfill_mailbox_t;

    backfiller_business_card_t() { }
    backfiller_business_card_t(
            const backfill_mailbox_t::address_t &ba,
            const cancel_backfill_mailbox_t::address_t &cba) :
        backfill_mailbox(ba), cancel_backfill_mailbox(cba)
        { }

    backfill_mailbox_t::address_t backfill_mailbox;
    cancel_backfill_mailbox_t::address_t cancel_backfill_mailbox;
};

RDB_DECLARE_SERIALIZABLE(backfiller_business_card_t);
RDB_DECLARE_EQUALITY_COMPARABLE(backfiller_business_card_t);

struct replica_bcard_t {
    /* This mailbox is used to ensure that the replica is at least as up to date as the
    timestamp. The second argument is used as an ack mailbox; the replica will send a
    reply there once it's at least as up to date as the timestamp. */
    typedef mailbox_t<void(
        state_timestamp_t,
        mailbox_addr_t<void()>
        )> synchronize_mailbox_t;

    synchronize_mailbox_t::address_t synchronize_mailbox;
    branch_id_t branch_id;
    backfiller_business_card_t backfiller_bcard;
};

RDB_DECLARE_SERIALIZABLE(replica_bcard_t);
RDB_DECLARE_EQUALITY_COMPARABLE(replica_bcard_t);

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_ */

