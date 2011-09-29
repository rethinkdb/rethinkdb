#ifndef __CLUSTERING_IMMEDIATE_CONSISTENCY_REPLIER_HPP__
#define __CLUSTERING_IMMEDIATE_CONSISTENCY_REPLIER_HPP__

#include "clustering/immediate_consistency/listener.hpp"

/* If you construct a `replier_t` for a given `listener_t`, then the listener
will inform the `broadcaster_t` that it's ready to reply to queries, and will
advertise via the metadata that it can perform backfills. When you destroy the
`replier_t`, the listener will tell the `broadcaster_t` that it's no longer
ready to reply to queries, and it will also stop performing backfills. */

template<class protocol_t>
class replier_t {

public:
    replier_t(listener_t<protocol_t> *l) :
        listener(l),
        stopped_replying(false),
        warm_shutdown_mailbox(listener->cluster, boost::bind(&cond_t::pulse, &warm_shutdown_cond)),
        backfiller(
            listener->cluster,
            listener->namespace_metadata,
            listener->store,
            metadata_new_member(generate_uuid(),
                metadata_field(&branch_metadata_t<protocol_t>::backfillers,
                    metadata_member(listener->branch_id,
                        metadata_field(&namespace_metadata_t<protocol_t>::branches,
                            listener->namespace_metadata
                ))))
            )
    {
        rassert(!listener->get_outdated_signal()->is_pulsed());

        send(listener->cluster,
            listener->registration_result_cond.get_value()->upgrade_mailbox,
            listener->writeread_mailbox.get_address(),
            listener->read_mailbox.get_address()
            );
    }

    /* The destructor immediately stops responding to queries. If there was an
    outstanding write or read that the broadcaster was expecting us to respond
    to, an error may be returned to the client.

    The destructor also immediately stops any outstanding backfills. */
    ~replier_t() {
        if (!stopped_replying) {
            send(listener->cluster,
                listener->registration_result_cond.get_value()->downgrade_mailbox,
                /* We don't want a confirmation */
                async_mailbox_t<void()>::address_t()
                );
        }
    }

    /* `warm_shutdown()` asks the broadcaster not to ask us to respond to reads
    or writes, but doesn't interrupt reads or writes that we have already been
    asked to respond to. When the `signal_t *` returned by `warm_shutdown()` is
    pulsed, all the pending reads or writes are complete and it's safe to
    destroy the `replier_t`.

    `warm_shutdown()` has no effect on backfills. */
    signal_t *warm_shutdown() {
        ASSERT_FINITE_CORO_WAITING;
        rassert(!stopped_replying);
        stopped_replying = true;
        send(listener->cluster,
            listener->registration_result_cond.get_value()->downgrade_mailbox,
            warm_shutdown_mailbox.get_address()
            );
        return &warm_shutdown_cond;
    }

private:
    listener_t<protocol_t> *listener;

    bool stopped_replying;
    cond_t warm_shutdown_cond;
    async_mailbox_t<void()> warm_shutdown_mailbox;

    backfiller_t<protocol_t> backfiller;
};

#endif /* __CLUSTERING_IMMEDIATE_CONSISTENCY_REPLIER_HPP__ */
