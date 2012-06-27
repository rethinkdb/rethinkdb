#include "errors.hpp"
#include <boost/variant.hpp>

#include "clustering/immediate_consistency/branch/backfillee.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/promise.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "containers/death_runner.hpp"

#define ALLOCATION_CHUNK 50

template <class backfill_chunk_t>
void push_chunk_on_queue(fifo_enforcer_queue_t<std::pair<std::pair<bool, backfill_chunk_t>, fifo_enforcer_write_token_t> > *queue, backfill_chunk_t chunk, fifo_enforcer_write_token_t token) {
    queue->push(token, std::make_pair(std::make_pair(true, chunk), token));
}

template <class backfill_chunk_t>
void push_finish_on_queue(fifo_enforcer_queue_t<std::pair<std::pair<bool, backfill_chunk_t>, fifo_enforcer_write_token_t> > *queue, fifo_enforcer_write_token_t token) {
    queue->push(token, std::make_pair(std::make_pair(false, backfill_chunk_t()), token));
}


/* Now that the metadata indicates that the backfill is happening, it's
   time to start actually performing backfill chunks */
template <class protocol_t>
class chunk_callback_t : public coro_pool_t<std::pair<std::pair<bool, typename protocol_t::backfill_chunk_t>, fifo_enforcer_write_token_t> >::callback_t,
                         public home_thread_mixin_t {
public:
    chunk_callback_t(multistore_ptr_t<protocol_t> *_svs,
                     fifo_enforcer_queue_t<std::pair<std::pair<bool, typename protocol_t::backfill_chunk_t>, fifo_enforcer_write_token_t> > *_chunk_queue, mailbox_manager_t *_mbox_manager,
                     mailbox_addr_t<void(int)> _allocation_mailbox) :
        svs(_svs), chunk_queue(_chunk_queue), mbox_manager(_mbox_manager),
        allocation_mailbox(_allocation_mailbox), unacked_chunks(0),
        done_message_arrived(false), num_outstanding_chunks(0)
    { }

    void actually_call_receive_backfill(const std::vector<int> *indices,
                                        const std::vector<typename protocol_t::backfill_chunk_t> *sharded_chunks,
                                        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens,
                                        signal_t *interruptor,
                                        int i) {
        try {
            rassert(write_tokens[i].get() != NULL);
            rassert(interruptor != NULL);

            svs->get_store_view((*indices)[i])->receive_backfill((*sharded_chunks)[i], write_tokens[i], interruptor);
        } catch (interrupted_exc_t&) {
            // We check if the thing was pulsed after pmap returns.
        }
    }

    void apply_backfill_chunk(fifo_enforcer_write_token_t chunk_token, const typename protocol_t::backfill_chunk_t& chunk, signal_t *interruptor) {
        const int num_stores = svs->num_stores();

        std::vector<int> indices;
        std::vector<typename protocol_t::backfill_chunk_t> sharded_chunks;

        typename protocol_t::region_t chunk_region = chunk.get_region();

        for (int i = 0; i < num_stores; ++i) {
            typename protocol_t::region_t region = svs->get_region(i);

            typename protocol_t::region_t intersection = region_intersection(region, chunk_region);
            if (!region_is_empty(intersection)) {
                indices.push_back(i);
                sharded_chunks.push_back(chunk.shard(intersection));
            }
        }

        boost::scoped_array<boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> > write_tokens(new boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t>[indices.size()]);
        svs->new_particular_write_tokens(indices.data(), indices.size(), write_tokens.get());
        chunk_queue->finish_write(chunk_token);

        pmap(indices.size(), boost::bind(&chunk_callback_t::actually_call_receive_backfill, this, &indices, &sharded_chunks, write_tokens.get(), interruptor, _1));

        if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }
    }

    // TODO: Don't pass a std::pair of std::pair.
    void coro_pool_callback(std::pair<std::pair<bool, typename protocol_t::backfill_chunk_t>, fifo_enforcer_write_token_t> chunk, signal_t *interruptor) {
        assert_thread();
        try {
            if (chunk.first.first) {
                /* This is an actual backfill chunk */

                /* Before letting the next thing go, increment
                   `num_outstanding_chunks` and acquire a write token. The
                   former is so that if the next thing is a done message,
                   it won't pulse `done_cond` while we're still going. The
                   latter is so that the backfill chunks acquire the
                   superblock in the correct order. */
                num_outstanding_chunks++;

                // We acquire the write token in apply_backfill_chunk.
                apply_backfill_chunk(chunk.second, chunk.first.second, interruptor);

                /* Allow the backfiller to send us more data */
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

                num_outstanding_chunks--;

            } else {
                /* This is a fake backfill "chunk" that just indicates
                   that the backfill is over */
                rassert(!done_message_arrived);
                done_message_arrived = true;
                chunk_queue->finish_write(chunk.second);
            }

            if (done_message_arrived && num_outstanding_chunks == 0) {
                done_cond.pulse_if_not_already_pulsed();
            }

        } catch (interrupted_exc_t) {
            /* This means that the `coro_pool_t` is being destroyed
               before the queue drains. That can only happen if we are
               being interrupted or if we lost contact with the backfiller.
               In either case, abort; the store will be left in a
               half-backfilled state. */
        }
    }

    cond_t done_cond;

private:
    multistore_ptr_t<protocol_t> *svs;
    fifo_enforcer_queue_t<std::pair<std::pair<bool, typename protocol_t::backfill_chunk_t>, fifo_enforcer_write_token_t> > *chunk_queue;
    mailbox_manager_t *mbox_manager;
    mailbox_addr_t<void(int)> allocation_mailbox;
    int unacked_chunks;
    bool done_message_arrived;
    int num_outstanding_chunks;

    DISABLE_COPYING(chunk_callback_t);
};

template <class protocol_t>
void receive_end_point_message(promise_t<std::pair<region_map_t<protocol_t, version_range_t>, branch_history_t<protocol_t> > > *promise,
        const region_map_t<protocol_t, version_range_t> &end_point,
        const branch_history_t<protocol_t> &associated_branch_history) {
    promise->pulse(std::make_pair(end_point, associated_branch_history));
}

template<class protocol_t>
void backfillee(
        mailbox_manager_t *mailbox_manager,
        branch_history_manager_t<protocol_t> *branch_history_manager,
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

    branch_history_t<protocol_t> start_point_associated_history;
    branch_history_manager->export_branch_history(start_point, &start_point_associated_history);

    /* The backfiller will send a message to `end_point_mailbox` before it sends
    any other messages; that message will tell us what the version will be when
    the backfill is over. */
    promise_t<std::pair<region_map_t<protocol_t, version_range_t>, branch_history_t<protocol_t> > > end_point_cond;
    mailbox_t<void(region_map_t<protocol_t, version_range_t>, branch_history_t<protocol_t>)> end_point_mailbox(
        mailbox_manager,
        boost::bind(&receive_end_point_message<protocol_t>, &end_point_cond, _1, _2),
        mailbox_callback_mode_inline);

    {
        typedef typename protocol_t::backfill_chunk_t backfill_chunk_t;

        /* A queue of the requests the backfill chunk mailbox receives, a coro
         * pool services these requests and poops them off one at a time to
         * perform them. */

        typedef fifo_enforcer_queue_t<std::pair<std::pair<bool, backfill_chunk_t>, fifo_enforcer_write_token_t> > chunk_queue_t; 
        chunk_queue_t chunk_queue;

        /* The backfiller will notify `done_mailbox` when the backfill is all over
        and the version described in `end_point_mailbox` has been achieved. */
        mailbox_t<void(fifo_enforcer_write_token_t)> done_mailbox(
            mailbox_manager,
            boost::bind(&push_finish_on_queue<backfill_chunk_t>, &chunk_queue, _1),
            mailbox_callback_mode_inline);

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
            start_point, start_point_associated_history,
            end_point_mailbox.get_address(),
            chunk_mailbox.get_address(),
            done_mailbox.get_address(),
            alloc_registration_mbox.get_address()
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

        /* Wait to get an allocation mailbox */
        wait_interruptible(alloc_mailbox_promise.get_ready_signal(), interruptor);

        mailbox_addr_t<void(int)> allocation_mailbox = alloc_mailbox_promise.get_value();

        /* Wait until we get a message in `end_point_mailbox`. */
        {
            wait_any_t waiter(end_point_cond.get_ready_signal(), backfiller.get_failed_signal());
            wait_interruptible(&waiter, interruptor);

            /* Throw an exception if backfiller died */
            backfiller.access();
            rassert(end_point_cond.get_ready_signal()->is_pulsed());
        }

        /* Record the updated branch history information that we got. It's
        essential that we call `record_branch_history()` before we update the
        metainfo, because otherwise if we crashed at a bad time the data might
        make it to disk as part of the metainfo but not as part of the branch
        history, and that would lead to crashes. */
        branch_history_manager->import_branch_history(end_point_cond.get_value().second, interruptor);

        /* Indicate in the metadata that a backfill is happening. We do this by
        marking every region as indeterminate between the current state and the
        backfill end state, since we don't know whether the backfill has reached
        that region yet. */

        typedef region_map_t<protocol_t, version_range_t> version_map_t;

        version_map_t end_point = end_point_cond.get_value().first;

        std::vector<std::pair<typename protocol_t::region_t, version_range_t> > span_parts;

        for (typename version_map_t::const_iterator it  = start_point.begin();
                                                    it != start_point.end();
                                                    it++) {
            for (typename version_map_t::const_iterator jt  = end_point.begin();
                                                        jt != end_point.end();
                                                        jt++) {
                typename protocol_t::region_t ixn = region_intersection(it->first, jt->first);
                if (!region_is_empty(ixn)) {
                    rassert(version_is_ancestor(branch_history_manager,
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

        chunk_callback_t<protocol_t> chunk_callback(svs, &chunk_queue, mailbox_manager, allocation_mailbox);

        coro_pool_t<std::pair<std::pair<bool, backfill_chunk_t>, fifo_enforcer_write_token_t> > backfill_workers(10, &chunk_queue, &chunk_callback);

        /* Now wait for the backfill to be over */
        {
            wait_any_t waiter(&chunk_callback.done_cond, backfiller.get_failed_signal());
            wait_interruptible(&waiter, interruptor);

            /* Throw an exception if backfiller died */
            backfiller.access();

            rassert(chunk_callback.done_cond.is_pulsed());
        }

        /* All went well, so don't send a cancel message to the backfiller */
        backfiller_notifier.fun = 0;
    }

    /* Update the metadata to indicate that the backfill occurred */
    boost::scoped_array< boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> > write_tokens(new boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t>[num_stores]);

    svs->new_write_tokens(write_tokens.get(), num_stores);

    svs->set_all_metainfos(
        region_map_transform<protocol_t, version_range_t, binary_blob_t>(
            end_point_cond.get_value().first,
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
        branch_history_manager_t<mock::dummy_protocol_t> *branch_history_manager,
        multistore_ptr_t<mock::dummy_protocol_t> *svs,
        mock::dummy_protocol_t::region_t region,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t<mock::dummy_protocol_t> > > > > backfiller_metadata,
        backfill_session_id_t backfill_session_id,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t);

template void backfillee<memcached_protocol_t>(
        mailbox_manager_t *mailbox_manager,
        branch_history_manager_t<memcached_protocol_t> *branch_history_manager,
        multistore_ptr_t<memcached_protocol_t> *svs,
        memcached_protocol_t::region_t region,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t<memcached_protocol_t> > > > > backfiller_metadata,
        backfill_session_id_t backfill_session_id,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t);
