// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_

#include "clustering/immediate_consistency/history.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/mailbox/typed.hpp"

/* `backfiller_bcard_t` represents a thing that is willing to serve backfills over the
network. It appears in the directory. */
class backfiller_bcard_t {
public:
    /* The backfillee sends the backfiller a `backfiller_bcard_t::intro_1_t` to start the
    backfill. The backfiller responds with a `backfiller_bcard_t::intro_2_t`. */

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        std::deque<backfill_pre_chunk_t>,
        region_map_t<state_timestamp_t>
        )> pre_chunk_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        std::deque<backfill_chunk_t>,
        region_map_t<version_t>
        )> chunk_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        key_range_t,
        region_map_t<state_timestamp_t>,
        chunk_mailbox_t::address_t
        )> start_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t
        )> stop_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t
        )> continue_mailbox_t;

    class intro_2_t {
    public:
        pre_chunk_mailbox_t::address_t pre_chunk_mailbox;
        start_mailbox_t::address_t start_mailbox;
        stop_mailbox_t::address_t stop_mailbox;
        continue_mailbox_t::address_t continue_mailbox;
    };

    class intro_1_t {
    public:
        mailbox_t<void(intro2_t)>::address_t intro_mailbox;
    };

    region_t region;
    registrar_business_card_t<intro1_t> registrar;
};

RDB_DECLARE_SERIALIZABLE(backfiller_bcard_t);
RDB_DECLARE_EQUALITY_COMPARABLE(backfiller_bcard_t);

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_ */

