#ifndef __CLUSTERING_BACKFILLEE_HPP__
#define __CLUSTERING_BACKFILLEE_HPP__

#include "clustering/backfill_metadata.hpp"

template<class protocol_t>
void backfill(
    typename protocol_t::store_t *store,
    mailbox_cluster_t *cluster,
    metadata_read_view_t<resource_metadata_t<backfiller_metadata_t<protocol_t> > > *backfiller_md,
    signal_t *interruptor) {

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    /* Start watching to see if the backfill provider dies or goes offline at
    any point */

    resource_access_t<backfiller_metadata_t<protocol_t> > backfiller(
        cluster, backfiller_md);

    /* Pick a unique ID to identify this backfill session to the backfiller */

    backfiller_metadata_t<protocol_t>::backfill_session_id_t backfill_session_id = generate_uuid();

    /* Begin the backfill. Once we call `begin_backfill()`, we have an
    obligation to call either `end_backfill()` or `cancel_backfill()`; the
    purpose of `backfill_cancel_notifier` is to make sure that
    `cancel_backfill()` gets called in the case of an exception. */

    death_runner_t backfill_cancel_notifier;
    backfill_cancel_notifier.fun = boost::bind(&protocol_t::store_t::cancel_backfill, store);
    typename protocol_t::backfill_request_t request = store->begin_backfill();

    /* Set up mailboxes and send off the backfill request */

    async_mailbox_t<void(typename protocol_t::backfill_chunk_t)> backfill_mailbox(
        cluster,
        /* TODO: `backfill_chunk()` has an inconsistent name. */
        boost::bind(&protocol_t::store_t::backfill_chunk, store, _1)
        );

    cond_t backfill_is_done;
    async_mailbox_t<void()> backfill_done_mailbox(cluster,
        boost::bind(&cond_t::pulse, &backfill_is_done));

    send(cluster,
        backfiller.access().backfill_mailbox,
        backfill_session_id,
        request,
        &backfill_mailbox,
        &backfill_done_mailbox);

    /* If something goes wrong, we'd like to inform the backfiller that it has
    gone wrong. `backfiller_notifier` notifies the backfiller in its destructor.
    If everything goes right, we'll explicitly disarm it. */

    death_runner_t backfiller_notifier;
    backfiller_notifier.fun = boost::bind(
        &send, cluster,
        backfiller.access().cancel_backfill_mailbox,
        backfill_session_id);

    /* Wait until either the backfill is over or something goes wrong */

    cond_t c;
    cond_link_t continue_when_backfill_done(&backfill_is_done, &c);
    cond_link_t continue_when_backfiller_dies(backfiller.get_failed_signal(), &c);
    cond_link_t continue_when_interrupted(interruptor, &c);
    c.wait_lazily_unordered();

    /* If the we broke out because of the backfiller becoming inaccessible, this
    will throw an exception. */
    backfiller.access();

    if (interruptor->is_pulsed())
        throw interrupted_exc_t();

    rassert(backfill_is_done.is_pulsed());

    /* Everything went well, so don't send a cancel message to the backfiller.
    */

    backfiller_notifier.fun = 0;

    /* TODO: Make `end_backfill()` take an argument. */

    /* End the backfill. Since we'll be calling `end_backfill()`, we don't want
    to call `cancel_backfill()`, so reset `backfill_cancel_notifier`. */

    backfill_cancel_notifier.fun = 0;
    store->end_backfill();
}

#endif /* __CLUSTERING_BACKFILLEE_HPP__ */
