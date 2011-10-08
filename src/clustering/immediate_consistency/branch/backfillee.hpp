#ifndef __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLEE_HPP__
#define __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLEE_HPP__

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "concurrency/promise.hpp"

template<class protocol_t>
void on_receive_backfill_chunk(
        store_view_t<protocol_t> *store,
        signal_t *dont_go_until,
        typename protocol_t::backfill_chunk_t chunk,
        signal_t *interruptor,
        UNUSED auto_drainer_t::lock_t keepalive)
        THROWS_NOTHING
{
    {
        wait_any_t waiter(dont_go_until, interruptor);
        waiter.wait_lazily_unordered();
        if (interruptor->is_pulsed()) { 
            return;
        }
    }

    try {
        store->begin_write_transaction(interruptor)->receive_backfill(chunk, interruptor);
    } catch (interrupted_exc_t) {
        return;
    }
};

template<class protocol_t>
void backfillee(
        mailbox_cluster_t *cluster,
        boost::shared_ptr<metadata_read_view_t<namespace_branch_metadata_t<protocol_t> > > namespace_metadata,
        store_view_t<protocol_t> *store,
        boost::shared_ptr<metadata_read_view_t<resource_metadata_t<backfiller_metadata_t<protocol_t> > > > backfiller_metadata,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t)
{
    resource_access_t<backfiller_metadata_t<protocol_t> > backfiller(cluster, backfiller_metadata);

    /* Read the metadata to determine where we're starting from */
    region_map_t<protocol_t, version_range_t> start_point =
        region_map_transform<protocol_t, binary_blob_t, version_range_t>(
            store->begin_read_transaction(interruptor)->get_metadata(interruptor),
            &binary_blob_t::get<version_range_t>
            );

    /* Pick a unique ID to identify this backfill session to the backfiller */
    backfill_session_id_t backfill_session_id = generate_uuid();

    /* The backfiller will send a message to `end_point_mailbox` before it sends
    any other messages; that message will tell us what the version will be when
    the backfill is over. */
    promise_t<region_map_t<protocol_t, version_range_t> > end_point_cond;
    async_mailbox_t<void(region_map_t<protocol_t, version_range_t>)> end_point_mailbox(
        cluster, boost::bind(&promise_t<region_map_t<protocol_t, version_range_t> >::pulse, &end_point_cond, _1));

    /* `dont_go_until` prevents any backfill chunks from being applied before we
    update the store metadata to indicate that the backfill is in progress. */
    cond_t dont_go_until;

    /* The backfiller will notify `done_mailbox` when the backfill is all over
    and the version described in `end_point_mailbox` has been achieved. */
    cond_t done_cond;
    async_mailbox_t<void()> done_mailbox(
        cluster, boost::bind(&cond_t::pulse, &done_cond));

    {
        /* Use an `auto_drainer_t` to wait for all the bits of the backfill to
        finish being applied. Construct it before `chunk_mailbox` so that we
        don't get any chunks after `drainer` is destroyed. */
        auto_drainer_t drainer;

        /* The backfiller will send individual chunks of the backfill to
        `chunk_mailbox`. */
        async_mailbox_t<void(typename protocol_t::backfill_chunk_t)> chunk_mailbox(
            cluster, boost::bind(&on_receive_backfill_chunk<protocol_t>,
                store, &dont_go_until, _1, interruptor, auto_drainer_t::lock_t(&drainer)
                ));

        /* Send off the backfill request */
        send(cluster,
            backfiller.access().backfill_mailbox,
            backfill_session_id,
            start_point,
            end_point_mailbox.get_address(),
            chunk_mailbox.get_address(),
            done_mailbox.get_address()
            );

        /* If something goes wrong, we'd like to inform the backfiller that it
        it has gone wrong, so it doesn't just keep blindly sending us chunks.
        `backfiller_notifier` notifies the backfiller in its destructor. If
        everything goes right, we'll explicitly disarm it. */
        death_runner_t backfiller_notifier;
        {
            /* We have to cast `send()` to the correct type before we pass
            it to `boost::bind()`, or else C++ can't figure out which
            overload to use. */
            void (*send_cast_to_correct_type)(
                mailbox_cluster_t *,
                typename backfiller_metadata_t<protocol_t>::cancel_backfill_mailbox_t::address_t,
                const backfill_session_id_t &) = &send;
            backfiller_notifier.fun = boost::bind(
                send_cast_to_correct_type, cluster,
                backfiller.access().cancel_backfill_mailbox,
                backfill_session_id);
        }

        /* Wait until we get a message in `end_point_mailbox`. */
        {
            wait_any_t waiter(end_point_cond.get_ready_signal(), backfiller.get_failed_signal(), interruptor);
            waiter.wait_lazily_unordered();
            if (interruptor->is_pulsed()) {
                throw interrupted_exc_t();
            }
            /* Throw an exception if backfiller died */
            backfiller.access();
            rassert(end_point_cond.get_ready_signal()->is_pulsed());
        }

        /* Indicate in the metadata that a backfill is happening. We do this by
        marking every region as indeterminate between the current state and the
        backfill end state, since we don't know whether the backfill has reached
        that region yet. */
        std::vector<std::pair<typename protocol_t::region_t, version_range_t> > start_point_parts =
            start_point.get_as_pairs();
        std::vector<std::pair<typename protocol_t::region_t, version_range_t> > end_point_parts =
            end_point_cond.get_value().get_as_pairs();
        std::vector<std::pair<typename protocol_t::region_t, version_range_t> > span_parts;
        for (int i = 0; i < (int)start_point_parts.size(); i++) {
            for (int j = 0; j < (int)end_point_parts.size(); j++) {
                typename protocol_t::region_t ixn = region_intersection(start_point_parts[i].first, end_point_parts[j].first);
                if (!region_is_empty(ixn)) {
                    rassert(version_is_ancestor(namespace_metadata,
                        start_point_parts[i].second.earliest,
                        end_point_parts[j].second.latest,
                        ixn),
                        "We're on a different timeline than the backfiller, "
                        "but it somehow failed to notice.");
                    span_parts.push_back(std::make_pair(
                        ixn,
                        version_range_t(start_point_parts[i].second.earliest, end_point_parts[j].second.latest)
                        ));
                }
            }
        }
        store->begin_write_transaction(interruptor)->set_metadata(
            region_map_transform<protocol_t, version_range_t, binary_blob_t>(
                region_map_t<protocol_t, version_range_t>(span_parts),
                &binary_blob_t::make<version_range_t>
                )
            );

        /* Now that the metadata indicates that the backfill is happening, it's
        OK for backfill chunks to be applied */
        dont_go_until.pulse();

        /* Now wait for the backfill to be over */
        {
            wait_any_t waiter(&done_cond, backfiller.get_failed_signal(), interruptor);
            waiter.wait_lazily_unordered();
            if (interruptor->is_pulsed()) {
                throw interrupted_exc_t();
            }
            /* Throw an exception if backfiller died */
            backfiller.access();
            rassert(done_cond.is_pulsed());
        }

        /* All went well, so don't send a cancel message to the backfiller */
        backfiller_notifier.fun = 0;

        /* `drainer` destructor is run here; we block until all the chunks are
        done being applied. */
    }

    /* Update the metadata to indicate that the backfill occurred */
    store->begin_write_transaction(interruptor)->set_metadata(
        region_map_transform<protocol_t, version_range_t, binary_blob_t>(
            end_point_cond.get_value(),
            &binary_blob_t::make<version_range_t>
            )
        );
}

#endif /* __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLEE_HPP__ */
