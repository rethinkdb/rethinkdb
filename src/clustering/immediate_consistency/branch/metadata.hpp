// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_METADATA_HPP_

#include <map>
#include <utility>

#include "clustering/generic/registration_metadata.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/promise.hpp"
#include "containers/uuid.hpp"
#include "protocol_api.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "timestamps.hpp"

template <class> class listener_intro_t;

/* Every `listener_t` constructs a `listener_business_card_t` and sends it to
the `broadcaster_t`. */

template<class protocol_t>
class listener_business_card_t {

public:
    /* These are the types of mailboxes that the master uses to communicate with
    the mirrors. */

    typedef mailbox_t<void(typename protocol_t::write_t,
                           transition_timestamp_t,
                           order_token_t,
                           fifo_enforcer_write_token_t,
                           mailbox_addr_t<void()> ack_addr)> write_mailbox_t;

    typedef mailbox_t<void(typename protocol_t::write_t,
                           transition_timestamp_t,
                           order_token_t,
                           fifo_enforcer_write_token_t,
                           mailbox_addr_t<void(typename protocol_t::write_response_t)>,
                           write_durability_t)> writeread_mailbox_t;

    typedef mailbox_t<void(typename protocol_t::read_t,
                           state_timestamp_t,
                           order_token_t,
                           fifo_enforcer_read_token_t,
                           mailbox_addr_t<void(typename protocol_t::read_response_t)>)> read_mailbox_t;

    /* The master sends a single message to `intro_mailbox` at the
    very beginning. This tells the mirror what timestamp it's at, the
    master's cpu sharding subspace count, and also tells it where to
    send upgrade/downgrade messages. */

    typedef mailbox_t<void(typename writeread_mailbox_t::address_t,
                           typename read_mailbox_t::address_t)> upgrade_mailbox_t;

    typedef mailbox_t<void(mailbox_addr_t<void()>)> downgrade_mailbox_t;

    typedef mailbox_t<void(listener_intro_t<protocol_t>)> intro_mailbox_t;

    listener_business_card_t() { }
    listener_business_card_t(const typename intro_mailbox_t::address_t &im,
                             const typename write_mailbox_t::address_t &wm)
        : intro_mailbox(im), write_mailbox(wm) { }

    typename intro_mailbox_t::address_t intro_mailbox;
    typename write_mailbox_t::address_t write_mailbox;

    RDB_MAKE_ME_SERIALIZABLE_2(intro_mailbox, write_mailbox);
};


template <class protocol_t>
class listener_intro_t {
public:
    state_timestamp_t broadcaster_begin_timestamp;
    typename listener_business_card_t<protocol_t>::upgrade_mailbox_t::address_t upgrade_mailbox;
    typename listener_business_card_t<protocol_t>::downgrade_mailbox_t::address_t downgrade_mailbox;

    listener_intro_t() { }
    listener_intro_t(state_timestamp_t _broadcaster_begin_timestamp,
                     typename listener_business_card_t<protocol_t>::upgrade_mailbox_t::address_t _upgrade_mailbox,
                     typename listener_business_card_t<protocol_t>::downgrade_mailbox_t::address_t _downgrade_mailbox)
        : broadcaster_begin_timestamp(_broadcaster_begin_timestamp),
          upgrade_mailbox(_upgrade_mailbox), downgrade_mailbox(_downgrade_mailbox) { }



    RDB_MAKE_ME_SERIALIZABLE_3(broadcaster_begin_timestamp,
                               upgrade_mailbox, downgrade_mailbox);
};


/* `backfiller_business_card_t` represents a thing that is willing to serve
backfills over the network. It appears in the directory. */

template<class protocol_t>
struct backfiller_business_card_t {

    typedef mailbox_t< void(
        backfill_session_id_t,
        region_map_t<protocol_t, version_range_t>,
        branch_history_t<protocol_t>,
        mailbox_addr_t< void(
            region_map_t<protocol_t, version_range_t>,
            branch_history_t<protocol_t>
            ) >,
        mailbox_addr_t<void(typename protocol_t::backfill_chunk_t, fifo_enforcer_write_token_t)>,
        mailbox_t<void(fifo_enforcer_write_token_t)>::address_t,
        mailbox_t<void(mailbox_addr_t<void(int)>)>::address_t
        )> backfill_mailbox_t;

    typedef mailbox_t<void(backfill_session_id_t)> cancel_backfill_mailbox_t;


    /* Mailboxes used for requesting the progress of a backfill */
    typedef mailbox_t<void(backfill_session_id_t, mailbox_addr_t<void(std::pair<int, int>)>)> request_progress_mailbox_t;

    backfiller_business_card_t() { }
    backfiller_business_card_t(
            const typename backfill_mailbox_t::address_t &ba,
            const cancel_backfill_mailbox_t::address_t &cba,
            const request_progress_mailbox_t::address_t &pa) :
        backfill_mailbox(ba), cancel_backfill_mailbox(cba), request_progress_mailbox(pa)
        { }

    typename backfill_mailbox_t::address_t backfill_mailbox;
    cancel_backfill_mailbox_t::address_t cancel_backfill_mailbox;
    request_progress_mailbox_t::address_t request_progress_mailbox;

    RDB_MAKE_ME_SERIALIZABLE_3(backfill_mailbox, cancel_backfill_mailbox, request_progress_mailbox);
};

/* `broadcaster_business_card_t` is the way that listeners find the broadcaster.
It appears in the directory. */

template<class protocol_t>
struct broadcaster_business_card_t {

    broadcaster_business_card_t(branch_id_t bid,
            const branch_history_t<protocol_t> &bh,
            const registrar_business_card_t<listener_business_card_t<protocol_t> > &r) :
        branch_id(bid), branch_id_associated_branch_history(bh), registrar(r) { }

    broadcaster_business_card_t() { }

    branch_id_t branch_id;
    branch_history_t<protocol_t> branch_id_associated_branch_history;

    registrar_business_card_t<listener_business_card_t<protocol_t> > registrar;

    RDB_MAKE_ME_SERIALIZABLE_3(branch_id, branch_id_associated_branch_history, registrar);
};

template<class protocol_t>
struct replier_business_card_t {
    /* This mailbox is used to check that the replier is at least as up to date
     * as the timestamp. The second argument is used as an ack mailbox, once
     * synchronization is complete the replier will send a message to it. */
    typedef mailbox_t<void(state_timestamp_t, mailbox_addr_t<void()>)> synchronize_mailbox_t;
    synchronize_mailbox_t::address_t synchronize_mailbox;

    backfiller_business_card_t<protocol_t> backfiller_bcard;

    replier_business_card_t()
    { }

    replier_business_card_t(const synchronize_mailbox_t::address_t &_synchronize_mailbox, const backfiller_business_card_t<protocol_t> &_backfiller_bcard)
        : synchronize_mailbox(_synchronize_mailbox), backfiller_bcard(_backfiller_bcard)
    { }

    RDB_MAKE_ME_SERIALIZABLE_2(synchronize_mailbox, backfiller_bcard);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_METADATA_HPP_ */
