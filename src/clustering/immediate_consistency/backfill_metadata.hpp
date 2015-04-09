// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_METADATA_HPP_

#include "btree/backfill.hpp"
#include "clustering/generic/registration_metadata.hpp"
#include "clustering/immediate_consistency/backfill_item_seq.hpp"
#include "clustering/immediate_consistency/history.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/mailbox/typed.hpp"

/* The backfiller publishes a `backfiller_bcard_t` which the backfillee uses to contact
it. The member types of `backfiller_bcard_t` describe the communications protocol between
the backfiller and the backfillee.

The backfillee begins by registering with the `registrar_t` described by the `registrar`
member of the `backfiller_bcard_t`. When it registers, it sends an `intro_1_t`, which
contains its initial version and a set of mailbox addresses. The initial version map
implicitly defines the range that the backfill will apply to. The backfiller compares the
backfillee's initial version with its own version and finds their closest common
ancestor. Then it sends back an `intro_2_t` via the `intro_1_t::intro_mailbox`. The
`intro_2_t` contains the common version which the backfiller calculated, along with its
own set of mailbox addresses.

Once the initial handshake is completed, two processes happen in parallel: the backfillee
transmits `backfill_pre_item_t`s to the backfiller, and the backfiller transmits
`backfill_item_t`s to the backfillee. These processes both proceed from left to right
lexicographically, and they are constrained to proceed at approximately the same pace:
The backfiller needs to know the pre-items for a given range before it can compute the
backfill items for that range, so the item traversal can't go ahead of the pre-item
traversal. The pre-item traversal can't go too far ahead of the item traversal because
the size of the pre-item queue is limited.

The item traversal process is broken into a series of sessions. (See `backfillee.hpp` for
an explanation of why; the sessions correspond to calls to `backfillee_t::go()`.) Each
session individually proceeds from left to right through some range of the key-space. The
sessions cannot overlap temporally, in the sense that no more than one session can exist
at the same time; but they may overlap in key-space, in the sense that the same key may
be backfilled multiple times as part of multiple sessions. The steps of each session are
as follows:

1. The backfillee begins a session by sending a message to the backfiller's
`begin_session_mailbox_t`, indicating where the session should start. The first session
must start at the left-hand side of the backfill range; subsequent sessions can start
anywhere between the left-hand side of the range and the furthest point where any session
has ended.

2. The backfiller starts traversing the B-tree; as it does so, it sends messages to the
`items_mailbox_t` on the backfillee. These messages form a contiguous series of
`backfill_item_seq_t`s in lexicographical order. Each message includes the corresponding
metainfo. The backfillee applies the items to its B-tree.

3. As the backfillee applies each item, it sends messages to the `ack_items_mailbox_t` on
the backfiller. The backfiller uses this as flow control; it limits the total mem size of
all of the items that it has sent but not received an acknowledgement for. (Note: The
backfillee must acknowledge every item, even if the session is interrupted.)

4a. The backfillee may decide to interrupt the session. It does this by sending a message
to the `end_session_mailbox_t` on the backfiller, which responds by sending a message to
the `ack_end_session_mailbox_t` on the backfillee. The backfillee must finish applying
any items it receives before the `ack_end_session_mailbox_t` message arrives. So the
backfiller knows that if it sends a message, the backfillee will apply it, even if the
session is interrupted (but not if the backfill is interrupted). Once the backfillee
applies all the backfill items and receives the `ack_end_session_mailbox_t`, the session
is over; go back to step 1.

4b. Alternatively, the backfiller might finish traversing the entire backfill range. When
the backfillee receives a `backfill_item_seq_t` that reaches all the way to the end of
the range, then it sends a message to the `end_session_mailbox_t` on the backfiller, to
which the backfiller responds with a message to the `ack_end_session_mailbox_t`, just
like in the other case. However, this doesn't mean that the backfill is over; the
backfillee may still choose to start another session from earlier in the key-range.

The pre-item traversal process is much simpler than the item traversal process. The
backfillee sends a series of messages to the `pre_items_mailbox_t` on the backfiller;
these messages form a contiguous series of `backfill_pre_item_t`s in lexicographical
order. The pre items are stored in a queue on the backfiller. As the backfiller traverses
each part of the key space for the first time, it consumes the corresponding pre-items
from the queue, freeing up space in the queue. It notifies the backfillee that there's
space in the pre items queue by sending messages to the `ack_pre_items_mailbox_t` on the
backfillee. A subsequent session might go over the same region of the key-space again;
but in this case, the backfiller doesn't need the pre items for that region of the key-
space, because it knows that the backfillee must have applied the items that it sent when
it traversed that part of the key-space the first time, and therefore there are no
changes present on the backfillee but not the backfiller for that key range. */
class backfiller_bcard_t {
public:
    /* All messages between the backfiller and the backfillee have
    `fifo_enforcer_write_token_t`s attached, which are used to keep them in order.
    Sometimes this isn't strictly necessary, since sometimes messages can be reordered
    without breaking anything; but it makes things simpler. */

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        backfill_item_seq_t<backfill_pre_item_t>
        )> pre_items_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        key_range_t::right_bound_t
        )> begin_session_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t
        )> end_session_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        /* This `size_t` has the same meaning as the numbers returned by
        `backfill_item_t::get_mem_size()`. */
        size_t
        )> ack_items_mailbox_t;

    class intro_2_t {
    public:
        region_map_t<state_timestamp_t> common_version;
        pre_items_mailbox_t::address_t pre_items_mailbox;
        begin_session_mailbox_t::address_t begin_session_mailbox;
        end_session_mailbox_t::address_t end_session_mailbox;
        ack_items_mailbox_t::address_t ack_items_mailbox;
    };

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        /* The `region_map_t` and the `backfill_item_seq_t` have the same region. */
        region_map_t<version_t>,
        backfill_item_seq_t<backfill_item_t>
        )> items_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t
        )> ack_end_session_mailbox_t;

    typedef mailbox_t<void(
        fifo_enforcer_write_token_t,
        /* This `size_t` has the same meaning as the numbers returned by
        `backfill_pre_item_t::get_mem_size()`. */
        size_t
        )> ack_pre_items_mailbox_t;

    class intro_1_t {
    public:
        region_map_t<version_t> initial_version;
        branch_history_t initial_version_history;
        mailbox_t<void(intro_2_t)>::address_t intro_mailbox;
        items_mailbox_t::address_t items_mailbox;
        ack_end_session_mailbox_t::address_t ack_end_session_mailbox;
        ack_pre_items_mailbox_t::address_t ack_pre_items_mailbox;
    };

    /* This `region_t` describes the region that the backfiller applies to. Backfill
    requests must cover a subset of this region's key-space, and they must cover exactly
    the same part of the hash-space as this region. */
    region_t region;

    registrar_business_card_t<intro_1_t> registrar;
};

RDB_DECLARE_SERIALIZABLE(backfiller_bcard_t::intro_2_t);
RDB_DECLARE_SERIALIZABLE(backfiller_bcard_t::intro_1_t);
RDB_DECLARE_SERIALIZABLE(backfiller_bcard_t);
RDB_DECLARE_EQUALITY_COMPARABLE(backfiller_bcard_t);

/* The `replica_t` publishes a `replica_bcard_t`, which the `remote_replicator_client_t`
uses to synchronize the backfills with the stream of writes it's getting from the
`primary_dispatcher_t`. So this isn't exactly part of the backfill itself, but it's in
this file for lack of a better place to put it. */
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

