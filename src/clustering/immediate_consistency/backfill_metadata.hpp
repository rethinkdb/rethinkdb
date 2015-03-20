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

    typedef uuid_u session_id_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        key_range_t,
        std::deque<backfill_pre_chunk_t>,
        )> pre_atoms_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        session_id_t,
        key_range_t
        )> go_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        session_id_t
        )> stop_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        session_id_t,
        key_range_t,
        size_t
        )> ack_atoms_mailbox_t;

    class intro_2_t {
    public:
        region_map_t<state_timestamp_t> common_version;
        pre_atoms_mailbox_t::address_t pre_atoms_mailbox;
        go_mailbox_t::address_t go_mailbox;
        stop_mailbox_t::address_t stop_mailbox;
        ack_atoms_mailbox_t::address_t ack_atoms_mailbox;
    };

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        session_id_t,
        region_map_t<version_t>,
        std::deque<backfill_chunk_t>
        )> atoms_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        size_t
        )> ack_pre_atoms_mailbox_t;

    class intro_1_t {
    public:
        region_map_t<version_t> initial_version;
        branch_history_t initial_version_history;
        mailbox_t<void(intro_2_t)>::address_t intro_mailbox;
        ack_pre_atoms_mailbox_t ack_pre_atoms_mailbox;
        atoms_mailbox_t atoms_mailbox;
    };

    region_t region;
    registrar_business_card_t<intro_1_t> registrar;
};

RDB_DECLARE_SERIALIZABLE(backfiller_bcard_t);
RDB_DECLARE_EQUALITY_COMPARABLE(backfiller_bcard_t);

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_ */

