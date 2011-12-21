#ifndef __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_REPLIER_HPP__
#define __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_REPLIER_HPP__

#include "clustering/immediate_consistency/branch/backfiller.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "rpc/metadata/view/field.hpp"
#include "rpc/metadata/view/member.hpp"

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
    replier_t(
            mailbox_cluster_t *cluster,
            boost::shared_ptr<metadata_readwrite_view_t<namespace_branch_metadata_t<protocol_t> > > namespace_metadata,
            listener_t<protocol_t> *l) :
        listener(l)
    {
        rassert(cluster == listener->cluster);
        rassert(listener->store->get_region() ==
            listener->namespace_metadata->get().branches[listener->branch_id].region,
            "Even though you can have a listener that only watches some subset "
            "of a branch, you can't have a replier for some subset of a "
            "branch.");
        rassert(!listener->get_outdated_signal()->is_pulsed());

        /* Notify the broadcaster that we can reply to queries */
        send(listener->cluster,
            listener->registration_result_cond.get_value()->upgrade_mailbox,
            listener->writeread_mailbox.get_address(),
            listener->read_mailbox.get_address()
            );

        /* Start serving backfills */
        backfiller_id = generate_uuid();
        backfiller.reset(new backfiller_t<protocol_t>(
            listener->cluster,
            listener->namespace_metadata,
            listener->store,
            metadata_new_member(backfiller_id,
                metadata_field(&branch_metadata_t<protocol_t>::backfillers,
                    metadata_member(listener->branch_id,
                        metadata_field(&namespace_branch_metadata_t<protocol_t>::branches,
                            namespace_metadata
                ))))
            ));

        stop_backfilling_if_listener_outdated.reset(new signal_t::subscription_t(
            boost::bind(&replier_t::on_listener_outdated, this),
            listener->get_outdated_signal()
            ));
    }

    /* The destructor immediately stops responding to queries. If there was an
    outstanding write or read that the broadcaster was expecting us to respond
    to, an error may be returned to the client.

    The destructor also immediately stops any outstanding backfills. */
    ~replier_t() {
        if (listener->get_outdated_signal()->is_pulsed()) {
            send(listener->cluster,
                listener->registration_result_cond.get_value()->downgrade_mailbox,
                /* We don't want a confirmation */
                async_mailbox_t<void()>::address_t()
                );
        }
    }

    backfiller_id_t get_backfiller_id() {
        rassert(!listener->get_outdated_signal()->is_pulsed(),
            "We're no longer serving backfills because we're outdated.");
        return backfiller_id;
    }

    /* TODO: Support warm shutdowns? */

private:
    void on_listener_outdated() {
        /* If the listener goes outdated, stop serving backfills. This is mostly
        a courtesy to newly-started listeners; if we serve them a backfill even
        though we're outdated, then they'll be outdated too.
        TODO: It would be nice if we could warm-shutdown the backfiller in this
        case. */
        backfiller.reset();
    }

    listener_t<protocol_t> *listener;

    backfiller_id_t backfiller_id;
    boost::scoped_ptr<backfiller_t<protocol_t> > backfiller;
    boost::scoped_ptr<signal_t::subscription_t> stop_backfilling_if_listener_outdated;
};

#endif /* __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_REPLIER_HPP__ */
