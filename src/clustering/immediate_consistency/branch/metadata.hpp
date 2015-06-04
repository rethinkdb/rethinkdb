// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_METADATA_HPP_

#include <map>
#include <utility>

#include "clustering/generic/registration_metadata.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/min_timestamp_enforcer.hpp"
#include "concurrency/promise.hpp"
#include "containers/uuid.hpp"
#include "protocol_api.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "timestamps.hpp"
#include "rdb_protocol/protocol.hpp" // ATN TODO why is this header needed?

class listener_intro_t;

/* Every `listener_t` constructs a `listener_business_card_t` and sends it to
the `broadcaster_t`. */

class listener_business_card_t {
public:
    /* These are the types of mailboxes that the master uses to communicate with
    the mirrors. */

    typedef mailbox_t<void(write_t,
                           state_timestamp_t,
                           order_token_t,
                           fifo_enforcer_write_token_t,
                           mailbox_addr_t<void()> ack_addr)> write_mailbox_t;

    typedef mailbox_t<void(write_t,
                           state_timestamp_t,
                           order_token_t,
                           fifo_enforcer_write_token_t,
                           mailbox_addr_t<void(write_response_t)>,
                           write_durability_t)> writeread_mailbox_t;

    typedef mailbox_t<void(read_t,
                           min_timestamp_token_t,
                           mailbox_addr_t<void(read_response_t)>)> read_mailbox_t;

    /* The master sends a single message to `intro_mailbox` at the
    very beginning. This tells the mirror what timestamp it's at, the
    master's cpu sharding subspace count, and also tells it where to
    send upgrade/downgrade messages. */

    typedef mailbox_t<void(writeread_mailbox_t::address_t,
                           read_mailbox_t::address_t)> upgrade_mailbox_t;

    typedef mailbox_t<void(mailbox_addr_t<void()>)> downgrade_mailbox_t;

    typedef mailbox_t<void(listener_intro_t)> intro_mailbox_t;

    listener_business_card_t() { }
    listener_business_card_t(const intro_mailbox_t::address_t &im,
                             const write_mailbox_t::address_t &wm,
                             const server_id_t &si)
        : intro_mailbox(im), write_mailbox(wm), server_id(si) { }

    intro_mailbox_t::address_t intro_mailbox;
    write_mailbox_t::address_t write_mailbox;
    server_id_t server_id;
};

RDB_DECLARE_SERIALIZABLE(listener_business_card_t);


class listener_intro_t {
public:
    state_timestamp_t broadcaster_begin_timestamp;
    listener_business_card_t::upgrade_mailbox_t::address_t upgrade_mailbox;
    listener_business_card_t::downgrade_mailbox_t::address_t downgrade_mailbox;
    uuid_u listener_id;

    listener_intro_t() { }
    listener_intro_t(state_timestamp_t _broadcaster_begin_timestamp,
                     listener_business_card_t::upgrade_mailbox_t::address_t _upgrade_mailbox,
                     listener_business_card_t::downgrade_mailbox_t::address_t _downgrade_mailbox,
                     uuid_u _listener_id)
        : broadcaster_begin_timestamp(_broadcaster_begin_timestamp),
          upgrade_mailbox(_upgrade_mailbox), downgrade_mailbox(_downgrade_mailbox),
          listener_id(_listener_id) { }
};

RDB_DECLARE_SERIALIZABLE(listener_intro_t);


/* `backfiller_business_card_t` represents a thing that is willing to serve
backfills over the network. It appears in the directory. */

struct backfiller_business_card_t {

    typedef mailbox_t< void(
        backfill_session_id_t,
        region_map_t<version_range_t>,
        branch_history_t,
        mailbox_addr_t< void(
            region_map_t<version_range_t>,
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

/* `broadcaster_business_card_t` is the way that listeners find the broadcaster.
It appears in the directory. */

struct broadcaster_business_card_t {

    broadcaster_business_card_t(branch_id_t bid,
            const branch_history_t &bh,
            const registrar_business_card_t<listener_business_card_t> &r) :
        branch_id(bid), branch_id_associated_branch_history(bh), registrar(r) { }

    broadcaster_business_card_t() { }

    branch_id_t branch_id;
    branch_history_t branch_id_associated_branch_history;

    registrar_business_card_t<listener_business_card_t> registrar;
};

RDB_DECLARE_SERIALIZABLE(broadcaster_business_card_t);
RDB_DECLARE_EQUALITY_COMPARABLE(broadcaster_business_card_t);

struct replier_business_card_t {
    /* This mailbox is used to check that the replier is at least as up to date
     * as the timestamp. The second argument is used as an ack mailbox, once
     * synchronization is complete the replier will send a message to it. */
    typedef mailbox_t<void(state_timestamp_t, mailbox_addr_t<void()>)> synchronize_mailbox_t;
    synchronize_mailbox_t::address_t synchronize_mailbox;

    backfiller_business_card_t backfiller_bcard;

    replier_business_card_t()
    { }

    replier_business_card_t(const synchronize_mailbox_t::address_t &_synchronize_mailbox, const backfiller_business_card_t &_backfiller_bcard)
        : synchronize_mailbox(_synchronize_mailbox), backfiller_bcard(_backfiller_bcard)
    { }
};

RDB_DECLARE_SERIALIZABLE(replier_business_card_t);
RDB_DECLARE_EQUALITY_COMPARABLE(replier_business_card_t);

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_METADATA_HPP_ */
