// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_REPLIER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_REPLIER_HPP_

#include "clustering/immediate_consistency/backfiller.hpp"
#include "clustering/immediate_consistency/metadata.hpp"

class listener_t;

/* If you construct a `replier_t` for a given `listener_t`, then the listener
will inform the `broadcaster_t` that it's ready to reply to queries, and will
advertise via the metadata that it can perform backfills. When you destroy the
`replier_t`, the listener will tell the `broadcaster_t` that it's no longer
ready to reply to queries, and it will also stop performing backfills. */

/* TODO: What if the upgrade/downgrade messages get reordered on the network?
For example, what if we create and then immediately destroy a `replier_t`, and
the downgrade message arrives at the `broadcaster_t` before the upgrade message
does? Consider using a FIFO enforcer or something. */

class replier_t {

public:
    replier_t(listener_t *l, mailbox_manager_t *mm, branch_history_manager_t *bhm);

    /* The destructor immediately stops responding to queries. If there was an
    outstanding write or read that the broadcaster was expecting us to respond
    to, an error may be returned to the client.

    The destructor also immediately stops any outstanding backfills. */
    ~replier_t();

    replier_business_card_t get_business_card();

    /* TODO: Support warm shutdowns? */

private:
    void on_synchronize(
        signal_t *interuptor,
        state_timestamp_t timestamp,
        mailbox_addr_t<void()> ack_mbox);

    mailbox_manager_t *mailbox_manager_;

    listener_t *listener_;

    replier_business_card_t::synchronize_mailbox_t synchronize_mailbox_;

    backfiller_t backfiller_;

    DISABLE_COPYING(replier_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_REPLIER_HPP_ */
