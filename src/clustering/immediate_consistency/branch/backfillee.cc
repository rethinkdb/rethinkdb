#include "errors.hpp"
#include <boost/variant.hpp>

#include "clustering/immediate_consistency/branch/backfillee.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/promise.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "containers/death_runner.hpp"

#define ALLOCATION_CHUNK 50

template<class protocol_t>
void on_receive_backfill_chunk(
        store_view_t<protocol_t> *store,
        signal_t *dont_go_until,
        typename protocol_t::backfill_chunk_t chunk,
        signal_t *interruptor,
        fifo_enforcer_write_token_t tok,
        fifo_enforcer_queue_t<std::pair<std::pair<bool, typename protocol_t::backfill_chunk_t>, fifo_enforcer_write_token_t> > *queue
        )
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
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> write_token;
        store->new_write_token(write_token);
        queue->finish_write(tok);
        store->receive_backfill(chunk, write_token, interruptor);
    } catch (interrupted_exc_t) {
        return;
    }
};

template <class backfill_chunk_t>
void push_chunk_on_queue(fifo_enforcer_queue_t<std::pair<std::pair<bool, backfill_chunk_t>, fifo_enforcer_write_token_t> > *queue, backfill_chunk_t chunk, fifo_enforcer_write_token_t token) {
    queue->push(token, std::make_pair(std::make_pair(true, chunk), token));
}

template <class backfill_chunk_t>
void push_finish_on_queue(fifo_enforcer_queue_t<std::pair<std::pair<bool, backfill_chunk_t>, fifo_enforcer_write_token_t> > *queue, fifo_enforcer_write_token_t token) {
    queue->push(token, std::make_pair(std::make_pair(false, backfill_chunk_t()), token));
}


template<class protocol_t>
void backfillee(
        mailbox_manager_t *mailbox_manager,
        UNUSED boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history,
        store_view_t<protocol_t> *store,
        typename protocol_t::region_t region,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > > > > backfiller_metadata,
        backfill_session_id_t backfill_session_id,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t)
{
    rassert(region_is_superset(store->get_region(), region));
    resource_access_t<backfiller_business_card_t<protocol_t> > backfiller(backfiller_metadata);

    /* Read the metadata to determine where we're starting from */
    boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> read_token;
    store->new_read_token(read_token);
    region_map_t<protocol_t, version_range_t> start_point =
        region_map_transform<protocol_t, binary_blob_t, version_range_t>(
            store->get_metainfo(read_token, interruptor),
            &binary_blob_t::get<version_range_t>
            );

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

    {
        typedef typename protocol_t::backfill_chunk_t backfill_chunk_t;

        /* A queue of the requests the backfill chunk mailbox receives, a coro
         * pool services these requests and poops them off one at a time to
         * perform them. */

        typedef fifo_enforcer_queue_t<std::pair<std::pair<bool, backfill_chunk_t>, fifo_enforcer_write_token_t> > chunk_queue_t; 
        chunk_queue_t chunk_queue;

        /* The backfiller will notify `done_mailbox` when the backfill is all over
        and the version described in `end_point_mailbox` has been achieved. */
        cond_t done_cond;
        mailbox_t<void(fifo_enforcer_write_token_t)> done_mailbox(
            mailbox_manager,
            boost::bind(&push_finish_on_queue<backfill_chunk_t>, &chunk_queue, _1),
            mailbox_callback_mode_inline);

        /* Use an `auto_drainer_t` to wait for all the bits of the backfill to
        finish being applied. Construct it before `chunk_mailbox` so that we
        don't get any chunks after `drainer` is destroyed. */
        auto_drainer_t drainer;

        /* The backfiller will send individual chunks of the backfill to
        `chunk_mailbox`. */
        mailbox_t<void(backfill_chunk_t, fifo_enforcer_write_token_t)> chunk_mailbox(
            mailbox_manager, boost::bind(&push_chunk_on_queue<backfill_chunk_t>, &chunk_queue, _1, _2));

        /* The backfiller will register for allocations on the allocation
         * registration box. */
        promise_t<mailbox_addr_t<void(int)> > alloc_mailbox_promise;
        mailbox_t<void(mailbox_addr_t<void(int)>)>  alloc_registration_mbox(
                mailbox_manager, boost::bind(&promise_t<mailbox_addr_t<void(int)> >::pulse, &alloc_mailbox_promise, _1), mailbox_callback_mode_inline);


        /* Send off the backfill request */
        send(mailbox_manager,
            backfiller.access().backfill_mailbox,
            backfill_session_id,
            start_point,
            end_point_mailbox.get_address(),
            chunk_mailbox.get_address(),
            done_mailbox.get_address(),
            alloc_registration_mbox.get_address()
            );

        /* Wait to get an allocation mailbox */
        wait_interruptible(alloc_mailbox_promise.get_ready_signal(), interruptor);

        mailbox_addr_t<void(int)> allocation_mailbox = alloc_mailbox_promise.get_value();

        /* Create a callback for popping backfill chunks off the queue and also
         * a queue of backfill chunks. */
        struct chunk_callback_t : public coro_pool_t<std::pair<std::pair<bool, backfill_chunk_t>, fifo_enforcer_write_token_t> >::callback_t, 
                                  public home_thread_mixin_t {
            chunk_callback_t(store_view_t<protocol_t> *_store, signal_t *_dont_go_until, 
                             signal_t *_interruptor, chunk_queue_t *_chunk_queue,
                             cond_t *_done_cond, mailbox_manager_t *_mbox_manager, 
                             mailbox_addr_t<void(int)> _allocation_mailbox)
                : store(_store), dont_go_until(_dont_go_until), interruptor(_interruptor), 
                  chunk_queue(_chunk_queue), done_cond(_done_cond), mbox_manager(_mbox_manager),
                  allocation_mailbox(_allocation_mailbox), unacked_chunks(0)
            { }
            void coro_pool_callback(std::pair<std::pair<bool, backfill_chunk_t>, fifo_enforcer_write_token_t> chunk) {
                assert_thread();
                if (chunk.first.first) {
                    on_receive_backfill_chunk(store, dont_go_until, chunk.first.second, interruptor, chunk.second, chunk_queue);
                    int chunks_to_send_out = 0;
                    {
                        /* Notice it's important that we don't wait while we're
                         * modifying unacked chunks, otherwise another callback may
                         * decided to send out an allocation as well. */
                        ASSERT_NO_CORO_WAITING;
                        ++unacked_chunks;
                        rassert(unacked_chunks <= ALLOCATION_CHUNK);
                        if (unacked_chunks == ALLOCATION_CHUNK) {
                            unacked_chunks = 0;
                            chunks_to_send_out = ALLOCATION_CHUNK;
                        }
                    }
                    if (chunks_to_send_out != 0) {
                        send(mbox_manager, allocation_mailbox, chunks_to_send_out);
                    }
                } else {
                    rassert(!done_cond->is_pulsed());
                    done_cond->pulse();
                }
            }
            store_view_t<protocol_t> *store;
            signal_t *dont_go_until;
            signal_t *interruptor;
            chunk_queue_t *chunk_queue;
            cond_t *done_cond;
            mailbox_manager_t *mbox_manager;
            mailbox_addr_t<void(int)> allocation_mailbox;
            int unacked_chunks;
        };

        /* The callback. */
        chunk_callback_t chunk_callback(store, &dont_go_until, interruptor, &chunk_queue, &done_cond, mailbox_manager, allocation_mailbox);

        /* A pool of coroutines that run the callback on the backfill chunks. */
        coro_pool_t<std::pair<std::pair<bool, backfill_chunk_t>, fifo_enforcer_write_token_t> > backfill_workers(10, &chunk_queue, &chunk_callback);

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
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> write_token;
        store->new_write_token(write_token);
        store->set_metainfo(
            region_map_transform<protocol_t, version_range_t, binary_blob_t>(
                region_map_t<protocol_t, version_range_t>(span_parts.begin(), span_parts.end()),
                &binary_blob_t::make<version_range_t>
                ),
            write_token,
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
    boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> write_token;
    store->new_write_token(write_token);
    store->set_metainfo(
        region_map_transform<protocol_t, version_range_t, binary_blob_t>(
            end_point_cond.get_value(),
            &binary_blob_t::make<version_range_t>
            ),
        write_token,
        interruptor
        );
}


#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"

template void backfillee<mock::dummy_protocol_t>(
        mailbox_manager_t *mailbox_manager,
        UNUSED boost::shared_ptr<semilattice_read_view_t<branch_history_t<mock::dummy_protocol_t> > > branch_history,
        store_view_t<mock::dummy_protocol_t> *store,
        mock::dummy_protocol_t::region_t region,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t<mock::dummy_protocol_t> > > > > backfiller_metadata,
        backfill_session_id_t backfill_session_id,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t);

template void on_receive_backfill_chunk<mock::dummy_protocol_t>(
        store_view_t<mock::dummy_protocol_t> *store,
        signal_t *dont_go_until,
        mock::dummy_protocol_t::backfill_chunk_t chunk,
        signal_t *interruptor,
        fifo_enforcer_write_token_t tok,
        fifo_enforcer_queue_t<std::pair<std::pair<bool, mock::dummy_protocol_t::backfill_chunk_t>, fifo_enforcer_write_token_t> > *queue)
    THROWS_NOTHING;


template void backfillee<memcached_protocol_t>(
        mailbox_manager_t *mailbox_manager,
        UNUSED boost::shared_ptr<semilattice_read_view_t<branch_history_t<memcached_protocol_t> > > branch_history,
        store_view_t<memcached_protocol_t> *store,
        memcached_protocol_t::region_t region,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t<memcached_protocol_t> > > > > backfiller_metadata,
        backfill_session_id_t backfill_session_id,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t);

template void on_receive_backfill_chunk<memcached_protocol_t>(
        store_view_t<memcached_protocol_t> *store,
        signal_t *dont_go_until,
        memcached_protocol_t::backfill_chunk_t chunk,
        signal_t *interruptor,
        fifo_enforcer_write_token_t tok,
        fifo_enforcer_queue_t<std::pair<std::pair<bool, memcached_protocol_t::backfill_chunk_t>, fifo_enforcer_write_token_t> > *queue)
    THROWS_NOTHING;
