#ifndef __CLUSTERING_BACKFILLEE_HPP__
#define __CLUSTERING_BACKFILLEE_HPP__

#include "clustering/backfill_metadata.hpp"
#include "rpc/metadata/view.hpp"
#include "clustering/resource.hpp"
#include "concurrency/promise.hpp"

template<class protocol_t>
boost::shared_ptr<ready_store_view_t<protocol_t> > backfill(
    mailbox_cluster_t *cluster,
    boost::shared_ptr<outdated_store_view_t<protocol_t> > store,
    std::vector<std::pair<typename protocol_t::region_t, version_t> > start_point,
    boost::shared_ptr<metadata_read_view_t<resource_metadata_t<backfiller_metadata_t<protocol_t> > > > backfiller_md,
    signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t)
{
    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    /* Start watching to see if the backfill provider dies or goes offline at
    any point */

    resource_access_t<backfiller_metadata_t<protocol_t> > backfiller(
        cluster, backfiller_md);

    /* Pick a unique ID to identify this backfill session to the backfiller */

    typename backfiller_metadata_t<protocol_t>::backfill_session_id_t backfill_session_id = generate_uuid();

    /* Set up mailboxes and send off the backfill request */

    async_mailbox_t<void(typename protocol_t::backfill_chunk_t)> backfill_mailbox(
        cluster, boost::bind(&outdated_store_view_t<protocol_t>::receive_backfill_chunk, store, _1));

    promise_t<state_timestamp_t> backfill_is_done;
    async_mailbox_t<void(state_timestamp_t)> backfill_done_mailbox(
        cluster, boost::bind(&promise_t<state_timestamp_t>::pulse, &backfill_is_done, _1));

    send(cluster,
        backfiller.access().backfill_mailbox,
        backfill_session_id,
        start_point,
        backfill_mailbox.get_address(),
        backfill_done_mailbox.get_address());

    /* If something goes wrong, we'd like to inform the backfiller that it has
    gone wrong. `backfiller_notifier` notifies the backfiller in its destructor.
    If everything goes right, we'll explicitly disarm it. */

    death_runner_t backfiller_notifier;
    {
        /* We have to cast `send()` to the correct type before we pass it to
        `boost::bind()`, or else C++ can't figure out which overload to use. */
        void (*send_cast_to_correct_type)(
            mailbox_cluster_t *,
            typename backfiller_metadata_t<protocol_t>::cancel_backfill_mailbox_t::address_t,
            const typename backfiller_metadata_t<protocol_t>::backfill_session_id_t &) = &send;
        backfiller_notifier.fun = boost::bind(
            send_cast_to_correct_type, cluster,
            backfiller.access().cancel_backfill_mailbox,
            backfill_session_id);
    }

    /* Wait until either the backfill is over or something goes wrong */

    wait_any_t waiter(backfill_is_done.get_ready_signal(), backfiller.get_failed_signal(), interruptor);
    waiter.wait_lazily_unordered();

    /* If the we broke out because of the backfiller becoming inaccessible, this
    will throw an exception. */
    backfiller.access();

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    /* Everything went well, so don't send a cancel message to the backfiller.
    */

    backfiller_notifier.fun = 0;

    /* End the backfill. */

    return store->done(backfill_is_done.get_value());
}

#endif /* __CLUSTERING_BACKFILLEE_HPP__ */
