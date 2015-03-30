// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_

#include "clustering/generic/registration_metadata.hpp"
#include "clustering/immediate_consistency/backfill_atom_seq.hpp"
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
        backfill_atom_seq_t<backfill_pre_atom_t>
        )> pre_atoms_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        key_range_t::right_bound_t
        )> begin_session_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        )> end_session_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
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
        backfill_atom_seq_t<backfill_pre_atom_t>
        )> atoms_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t
        )> ack_end_session_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        size_t
        )> ack_pre_atoms_mailbox_t;

    class intro_1_t {
    public:
        region_map_t<version_t> initial_version;
        branch_history_t initial_version_history;
        mailbox_t<void(intro_2_t)>::address_t intro_mailbox;
        atoms_mailbox_t::address_t atoms_mailbox;
        ack_end_session_mailbox_t::address_t ack_end_session_mailbox;
        ack_pre_atoms_mailbox_t::address_t ack_pre_atoms_mailbox;
    };

    region_t region;
    registrar_business_card_t<intro_1_t> registrar;
};

RDB_DECLARE_SERIALIZABLE(backfiller_bcard_t::intro_2_t);
RDB_DECLARE_SERIALIZABLE(backfiller_bcard_t::intro_1_t);
RDB_DECLARE_SERIALIZABLE(backfiller_bcard_t);
RDB_DECLARE_EQUALITY_COMPARABLE(backfiller_bcard_t);

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
    backfiller_bcard_t backfiller_bcard;
};

RDB_DECLARE_SERIALIZABLE(replica_bcard_t);
RDB_DECLARE_EQUALITY_COMPARABLE(replica_bcard_t);

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_ */

