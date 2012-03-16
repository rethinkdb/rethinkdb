#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_REPLIER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_REPLIER_HPP_

#include "clustering/immediate_consistency/branch/backfiller.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/view/member.hpp"
#include "clustering/immediate_consistency/branch/metadata.hpp"

/* If you construct a `replier_t` for a given `listener_t`, then the listener
will inform the `broadcaster_t` that it's ready to reply to queries, and will
advertise via the metadata that it can perform backfills. When you destroy the
`replier_t`, the listener will tell the `broadcaster_t` that it's no longer
ready to reply to queries, and it will also stop performing backfills. */

/* TODO: What if the upgrade/downgrade messages get reordered on the network?
For example, what if we create and then immediately destroy a `replier_t`, and
the downgrade message arrives at the `broadcaster_t` before the upgrade message
does? Consider using a FIFO enforcer or something. */

template<class protocol_t>
class replier_t {

public:
    explicit replier_t(listener_t<protocol_t> *l) :
        listener(l), 

        synchronize_mailbox(listener->mailbox_manager, 
                            boost::bind(&replier_t<protocol_t>::on_synchronize, 
                                        this, 
                                        _1, 
                                        _2, 
                                        auto_drainer_t::lock_t(&drainer))),

        /* Start serving backfills */
        backfiller(
            listener->mailbox_manager,
            listener->branch_history,
            listener->store
            )
    {
        rassert(listener->store->get_region() ==
            listener->branch_history->get().branches[listener->branch_id].region,
            "Even though you can have a listener that only watches some subset "
            "of a branch, you can't have a replier for some subset of a "
            "branch.");

        /* Notify the broadcaster that we can reply to queries */
        send(listener->mailbox_manager,
            listener->registration_done_cond.get_value().upgrade_mailbox,
            listener->writeread_mailbox.get_address(),
            listener->read_mailbox.get_address()
            );
    }

    /* The destructor immediately stops responding to queries. If there was an
    outstanding write or read that the broadcaster was expecting us to respond
    to, an error may be returned to the client.

    The destructor also immediately stops any outstanding backfills. */
    ~replier_t() {
        if (listener->get_broadcaster_lost_signal()->is_pulsed()) {
            send(listener->mailbox_manager,
                listener->registration_done_cond.get_value().downgrade_mailbox,
                /* We don't want a confirmation */
                async_mailbox_t<void()>::address_t()
                );
        }
    }

    replier_business_card_t<protocol_t> get_business_card() {
        return replier_business_card_t<protocol_t>(synchronize_mailbox.get_address(), backfiller.get_business_card());
    }

    /* TODO: Support warm shutdowns? */

private:
    void on_synchronize(state_timestamp_t timestamp, async_mailbox_t<void()>::address_t ack_mbox, auto_drainer_t::lock_t keepalive) {
        try {
            listener->wait_for_version(timestamp, keepalive.get_drain_signal());
            send(listener->mailbox_manager, ack_mbox);
        } catch (interrupted_exc_t) {
        }
    }

    listener_t<protocol_t> *listener;

    auto_drainer_t drainer;

    typename replier_business_card_t<protocol_t>::synchronize_mailbox_t synchronize_mailbox;

    backfiller_t<protocol_t> backfiller;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_REPLIER_HPP_ */
