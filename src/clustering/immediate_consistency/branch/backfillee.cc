#include "clustering/immediate_consistency/branch/backfillee.hpp"

#include "clustering/immediate_consistency/branch/history.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "concurrency/promise.hpp"
#include "containers/death_runner.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "concurrency/coro_pool.hpp"

/* TODO: What if the backfill chunks on the network get reordered in transit?
Even if the `protocol_t` can tolerate backfill chunks being reordered, it's
clearly not OK for the end-of-backfill message to be processed before the last
of the backfill chunks are. Consider using a FIFO enforcer or something. */

template <class protocol_t>
void actually_call_receive_backfill(multistore_ptr_t<protocol_t> *svs,
                                    std::vector<int> *indices,
                                    std::vector<typename protocol_t::backfill_chunk_t> *sharded_chunks,
                                    signal_t *interruptor,
                                    int i) {
    try {

        // TODO: as mentioned below this is pointless b.s. ordering.
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> write_token;
        svs->get_store_view((*indices)[i])->new_write_token(write_token);
        svs->get_store_view((*indices)[i])->receive_backfill((*sharded_chunks)[i], write_token, interruptor);

    } catch (interrupted_exc_t&) {
        // we check if the thing was pulsed after pmap.
    }
}

template<class protocol_t>
void on_receive_backfill_chunk(
        multistore_ptr_t<protocol_t> *svs,
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
        std::vector<int> indices;
        std::vector<typename protocol_t::backfill_chunk_t> sharded_chunks;

        typename protocol_t::region_t chunk_region = chunk.get_region();

        for (int i = 0, e = svs->num_stores(); i < e; ++i) {
            typename protocol_t::region_t region = svs->get_region(i);

            typename protocol_t::region_t intersection = region_intersection(region, chunk_region);
            if (!region_is_empty(intersection)) {
                indices.push_back(i);
                sharded_chunks.push_back(chunk.shard(intersection));
            }
        }

        // TODO: Ordering is broken here definitely, _especially_ with pmap, USE FIFO ENFORCER TOKENS OR

        pmap(indices.size(), boost::bind(&actually_call_receive_backfill<protocol_t>, svs, &indices, &sharded_chunks, interruptor, _1));

        if (interruptor->is_pulsed()) {
            // kill me
            throw interrupted_exc_t();
        }

        // here have a member pointer:  &std::vector<int>::at
    } catch (interrupted_exc_t) {
        return;
    }
};

template <class backfill_chunk_t>
void push_with_drain_lock(unlimited_fifo_queue_t<std::pair<backfill_chunk_t, auto_drainer_t::lock_t> > *queue, backfill_chunk_t chunk, auto_drainer_t::lock_t lock) {
    queue->push(std::make_pair(chunk, lock));
}


template<class protocol_t>
void backfillee(
        mailbox_manager_t *mailbox_manager,
        UNUSED boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history,
        multistore_ptr_t<protocol_t> *svs,
        typename protocol_t::region_t region,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > > > > backfiller_metadata,
        backfill_session_id_t backfill_session_id,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t)
{
    rassert(region_is_superset(svs->get_multistore_joined_region(), region));
    resource_access_t<backfiller_business_card_t<protocol_t> > backfiller(backfiller_metadata);

    /* Read the metadata to determine where we're starting from */
    const int num_stores = svs->num_stores();
    boost::scoped_array<boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> > read_tokens(new boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t>[svs->num_stores()]);
    svs->new_read_tokens(read_tokens.get(), num_stores);

    region_map_t<protocol_t, version_range_t> start_point =
	svs->get_all_metainfos(order_token_t::ignore, read_tokens.get(), num_stores, interruptor);

    start_point = start_point.mask(region);

    /* The backfiller will send a message to `end_point_mailbox` before it sends
    any other messages; that message will tell us what the version will be when
    the backfill is over. */
    promise_t<region_map_t<protocol_t, version_range_t> > end_point_cond;
    mailbox_t<void(region_map_t<protocol_t, version_range_t>)> end_point_mailbox(
        mailbox_manager,
        boost::bind(&promise_t<region_map_t<protocol_t, version_range_t> >::pulse, &end_point_cond, _1),
        mailbox_callback_mode_inline);

    /* `dont_go_until` prevents any backfill chunks from being applied before we
    update the store metadata to indicate that the backfill is in progress. */
    cond_t dont_go_until;

    /* The backfiller will notify `done_mailbox` when the backfill is all over
    and the version described in `end_point_mailbox` has been achieved. */
    cond_t done_cond;
    mailbox_t<void()> done_mailbox(
        mailbox_manager,
        boost::bind(&cond_t::pulse, &done_cond),
        mailbox_callback_mode_inline);

    {
        /* A queue of the requests the backfill chunk mailbox receives, a coro
         * pool services these requests and poops them off one at a time to
         * perform them. */

        typedef typename protocol_t::backfill_chunk_t backfill_chunk_t;
        unlimited_fifo_queue_t<std::pair<backfill_chunk_t, auto_drainer_t::lock_t> > chunk_queue;


        struct chunk_callback_t : public coro_pool_t<std::pair<backfill_chunk_t, auto_drainer_t::lock_t> >::callback_t {
            chunk_callback_t(multistore_ptr_t<protocol_t> *_svs, signal_t *_dont_go_until, signal_t *_interruptor)
                : svs(_svs), dont_go_until(_dont_go_until), interruptor(_interruptor)
            { }
            void coro_pool_callback(std::pair<backfill_chunk_t, auto_drainer_t::lock_t> chunk) {
                on_receive_backfill_chunk(svs, dont_go_until, chunk.first, interruptor, chunk.second);
            }
            multistore_ptr_t<protocol_t> *svs;
            signal_t *dont_go_until;
            signal_t *interruptor;
        };

        chunk_callback_t chunk_callback(svs, &dont_go_until, interruptor);
        /* A callback, this function will be called on backfill_chunk_ts as
         * they are popped off the queue. */
        //typename coro_pool_t<std::pair<backfill_chunk_t, auto_drainer_t::lock_t> >::boost_function_callback_t 
        //    chunk_callback(boost::bind(&on_receive_backfill_chunk<protocol_t>,
        //        store, &dont_go_until, boost::bind(&std::pair::first, _1), interruptor, boost::bind(&std::pair::second, _1)
        //        ));

        coro_pool_t<std::pair<backfill_chunk_t, auto_drainer_t::lock_t> > backfill_workers(10, &chunk_queue, &chunk_callback);

        /* Use an `auto_drainer_t` to wait for all the bits of the backfill to
        finish being applied. Construct it before `chunk_mailbox` so that we
        don't get any chunks after `drainer` is destroyed. */
        auto_drainer_t drainer;

        /* The backfiller will send individual chunks of the backfill to
        `chunk_mailbox`. */
        mailbox_t<void(backfill_chunk_t)> chunk_mailbox(
            mailbox_manager, boost::bind(&push_with_drain_lock<backfill_chunk_t>, &chunk_queue, _1, auto_drainer_t::lock_t(&drainer)));

        /* Send off the backfill request */
        send(mailbox_manager,
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
                mailbox_manager_t *,
                typename backfiller_business_card_t<protocol_t>::cancel_backfill_mailbox_t::address_t,
                const backfill_session_id_t &) = &send;
            backfiller_notifier.fun = boost::bind(
                send_cast_to_correct_type, mailbox_manager,
                backfiller.access().cancel_backfill_mailbox,
                backfill_session_id);
        }

        /* Wait until we get a message in `end_point_mailbox`. */
        {
            wait_any_t waiter(end_point_cond.get_ready_signal(), backfiller.get_failed_signal());
            wait_interruptible(&waiter, interruptor);

            /* Throw an exception if backfiller died */
            backfiller.access();
            rassert(end_point_cond.get_ready_signal()->is_pulsed());
        }

        /* Indicate in the metadata that a backfill is happening. We do this by
        marking every region as indeterminate between the current state and the
        backfill end state, since we don't know whether the backfill has reached
        that region yet. */

        typedef region_map_t<protocol_t, version_range_t> version_map_t;

        version_map_t end_point = end_point_cond.get_value();

        std::vector<std::pair<typename protocol_t::region_t, version_range_t> > span_parts;

        for (typename version_map_t::const_iterator it  = start_point.begin();
                                                    it != start_point.end();
                                                    it++) {
            for (typename version_map_t::const_iterator jt  = end_point.begin();
                                                        jt != end_point.end();
                                                        jt++) {
                typename protocol_t::region_t ixn = region_intersection(it->first, jt->first);
                if (!region_is_empty(ixn)) {
                    rassert(version_is_ancestor(branch_history->get(),
                        it->second.earliest,
                        jt->second.latest,
                        ixn),
                        "We're on a different timeline than the backfiller, "
                        "but it somehow failed to notice.");
                    span_parts.push_back(std::make_pair(
                        ixn,
                        version_range_t(it->second.earliest, jt->second.latest)
                        ));
                }
            }
        }

        boost::scoped_array< boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> > write_tokens(new boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t>[num_stores]);

        svs->new_write_tokens(write_tokens.get(), num_stores);

        svs->set_all_metainfos(
            region_map_transform<protocol_t, version_range_t, binary_blob_t>(
                region_map_t<protocol_t, version_range_t>(span_parts.begin(), span_parts.end()),
                &binary_blob_t::make<version_range_t>
                ),
            order_token_t::ignore,
            write_tokens.get(),
            num_stores,
            interruptor
            );

        /* Now that the metadata indicates that the backfill is happening, it's
        OK for backfill chunks to be applied */
        dont_go_until.pulse();

        /* Now wait for the backfill to be over */
        {
            wait_any_t waiter(&done_cond, backfiller.get_failed_signal());
            wait_interruptible(&waiter, interruptor);

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
    boost::scoped_array< boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> > write_tokens(new boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t>[num_stores]);

    svs->new_write_tokens(write_tokens.get(), num_stores);

    svs->set_all_metainfos(
        region_map_transform<protocol_t, version_range_t, binary_blob_t>(
            end_point_cond.get_value(),
            &binary_blob_t::make<version_range_t>
            ),
        order_token_t::ignore,
        write_tokens.get(),
        num_stores,
        interruptor
    );
}


#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"

template void backfillee<mock::dummy_protocol_t>(
        mailbox_manager_t *mailbox_manager,
        UNUSED boost::shared_ptr<semilattice_read_view_t<branch_history_t<mock::dummy_protocol_t> > > branch_history,
        multistore_ptr_t<mock::dummy_protocol_t> *store,
        mock::dummy_protocol_t::region_t region,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t<mock::dummy_protocol_t> > > > > backfiller_metadata,
        backfill_session_id_t backfill_session_id,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t);


template void backfillee<memcached_protocol_t>(
        mailbox_manager_t *mailbox_manager,
        UNUSED boost::shared_ptr<semilattice_read_view_t<branch_history_t<memcached_protocol_t> > > branch_history,
        multistore_ptr_t<memcached_protocol_t> *store,
        memcached_protocol_t::region_t region,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t<memcached_protocol_t> > > > > backfiller_metadata,
        backfill_session_id_t backfill_session_id,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t);

