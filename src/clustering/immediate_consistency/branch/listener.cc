// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/branch/listener.hpp"

#include "clustering/generic/registrant.hpp"
#include "clustering/generic/resource.hpp"
#include "clustering/immediate_consistency/branch/backfillee.hpp"
#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/cross_thread_signal.hpp"


/* `WRITE_QUEUE_CORO_POOL_SIZE` is the number of coroutines that will be used
when draining the write queue after completing a backfill. */
#define WRITE_QUEUE_CORO_POOL_SIZE 1000

/* When we have caught up to the master to within
`WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY` elements, then we consider ourselves
to be up-to-date. */
#define WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY 5

/* When we are draining the write queue, we allow new objects to be added to the
write queue at (this rate) * (the rate at which objects are being popped). If
this number is high, then the backfill will take longer but the cluster will
serve queries at a high rate during the backfill. If this number is low, then
the backfill will be faster but the cluster will only serve queries slowly
during the backfill. */
#define WRITE_QUEUE_SEMAPHORE_TRICKLE_FRACTION 0.5

#ifndef NDEBUG
template <class protocol_t>
struct version_leq_metainfo_checker_callback_t : public metainfo_checker_callback_t<protocol_t> {
public:
    explicit version_leq_metainfo_checker_callback_t(const state_timestamp_t& tstamp) : tstamp_(tstamp) { }

    void check_metainfo(const region_map_t<protocol_t, binary_blob_t>& metainfo, const typename protocol_t::region_t& region) const {
        region_map_t<protocol_t, binary_blob_t> masked = metainfo.mask(region);

        for (typename region_map_t<protocol_t, binary_blob_t>::const_iterator it = masked.begin(); it != masked.end(); ++it) {
            version_range_t range = binary_blob_t::get<version_range_t>(it->second);
            guarantee(range.earliest.timestamp == range.latest.timestamp);
            guarantee(range.latest.timestamp <= tstamp_);
        }
    }

private:
    state_timestamp_t tstamp_;

    DISABLE_COPYING(version_leq_metainfo_checker_callback_t);
};
#endif // NDEBUG

template <class protocol_t>
listener_t<protocol_t>::listener_t(const base_path_t &base_path,
                                   io_backender_t *io_backender,
                                   mailbox_manager_t *mm,
                                   clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster_metadata,
                                   branch_history_manager_t<protocol_t> *branch_history_manager,
                                   store_view_t<protocol_t> *svs,
                                   clone_ptr_t<watchable_t<boost::optional<boost::optional<replier_business_card_t<protocol_t> > > > > replier,
                                   backfill_session_id_t backfill_session_id,
                                   perfmon_collection_t *backfill_stats_parent,
                                   signal_t *interruptor,
                                   order_source_t *order_source)
        THROWS_ONLY(interrupted_exc_t, backfiller_lost_exc_t, broadcaster_lost_exc_t) :

    mailbox_manager_(mm),
    svs_(svs),
    uuid_(generate_uuid()),
    perfmon_collection_(),
    perfmon_collection_membership_(backfill_stats_parent, &perfmon_collection_, "backfill-serialization-" + uuid_to_str(uuid_)),
    /* TODO: Put the file in the data directory, not here */
    write_queue_(io_backender,
                 serializer_filepath_t(base_path, "backfill-serialization-" + uuid_to_str(uuid_)),
                 &perfmon_collection_),
    write_queue_semaphore_(SEMAPHORE_NO_LIMIT,
        WRITE_QUEUE_SEMAPHORE_TRICKLE_FRACTION),
    enforce_max_outstanding_writes_from_broadcaster_(MAX_OUTSTANDING_WRITES_FROM_BROADCASTER),
    write_mailbox_(mailbox_manager_,
        boost::bind(&listener_t::on_write, this, _1, _2, _3, _4, _5)),
    writeread_mailbox_(mailbox_manager_,
        boost::bind(&listener_t::on_writeread, this, _1, _2, _3, _4, _5, _6)),
    read_mailbox_(mailbox_manager_,
        boost::bind(&listener_t::on_read, this, _1, _2, _3, _4, _5))
{
    boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > business_card =
        broadcaster_metadata->get();
    if (!business_card || !business_card.get()) {
        throw broadcaster_lost_exc_t();
    }

    branch_id_ = business_card.get().get().branch_id;

    branch_birth_certificate_t<protocol_t> this_branch_history;

    {
        cross_thread_signal_t ct_interruptor(interruptor, branch_history_manager->home_thread());
        on_thread_t th(branch_history_manager->home_thread());
        branch_history_manager->import_branch_history(business_card.get().get().branch_id_associated_branch_history, interruptor);
        this_branch_history = branch_history_manager->get_branch(branch_id_);
    }

    our_branch_region_ = this_branch_history.region;

#ifndef NDEBUG
    /* Sanity-check to make sure we're on the same timeline as the thing
       we're trying to join. The backfiller will perform an equivalent check,
       but if there's an error it would be nice to catch it where the action
       was initiated. */

    rassert(region_is_superset(our_branch_region_, svs_->get_region()));

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t> read_token;
    svs_->new_read_token(&read_token);
    region_map_t<protocol_t, binary_blob_t> start_point_blob;
    svs_->do_get_metainfo(order_source->check_in("listener_t(A)").with_read_mode(), &read_token, interruptor, &start_point_blob);
    region_map_t<protocol_t, version_range_t> start_point = to_version_range_map(start_point_blob);

    {
        on_thread_t th(branch_history_manager->home_thread());
        for (typename region_map_t<protocol_t, version_range_t>::const_iterator it = start_point.begin();
             it != start_point.end();
             ++it) {

            version_t version = it->second.latest;
            rassert(version.branch == branch_id_ ||
                    version_is_ancestor(branch_history_manager,
                                        version,
                                        version_t(branch_id_, this_branch_history.initial_timestamp),
                                        it->first));
        }
    }
#endif

    /* Attempt to register for reads and writes */
    try_start_receiving_writes(broadcaster_metadata, interruptor);
    listener_intro_t<protocol_t> listener_intro;
    bool registration_is_done = registration_done_cond_.try_get_value(&listener_intro);
    guarantee(registration_is_done);

    state_timestamp_t streaming_begin_point = listener_intro.broadcaster_begin_timestamp;

    try {
        /* Go through a little song and dance to make sure that the
         * backfiller will at least get us to the point that we will being
         * live streaming from. */

        cond_t backfiller_is_up_to_date;
        mailbox_t<void()> ack_mbox(
            mailbox_manager_,
            boost::bind(&cond_t::pulse, &backfiller_is_up_to_date));

        resource_access_t<replier_business_card_t<protocol_t> > replier_access(replier);
        send(mailbox_manager_, replier_access.access().synchronize_mailbox, streaming_begin_point, ack_mbox.get_address());

        wait_any_t interruptor2(interruptor, replier_access.get_failed_signal());
        wait_interruptible(&backfiller_is_up_to_date, &interruptor2);

        /* Backfill */
        backfillee<protocol_t>(mailbox_manager_,
                               branch_history_manager,
                               svs_,
                               svs_->get_region(),
                               replier->subview(&listener_t<protocol_t>::get_backfiller_from_replier_bcard),
                               backfill_session_id,
                               interruptor);
    } catch (const resource_lost_exc_t &) {
        throw backfiller_lost_exc_t();
    }

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t> read_token2;
    svs_->new_read_token(&read_token2);

    region_map_t<protocol_t, binary_blob_t> backfill_end_point_blob;
    svs_->do_get_metainfo(order_source->check_in("listener_t(B)").with_read_mode(), &read_token2, interruptor, &backfill_end_point_blob);

    region_map_t<protocol_t, version_range_t> backfill_end_point = to_version_range_map(backfill_end_point_blob);

    /* Sanity checking. */

    /* Make sure the region is not empty. */
    guarantee(backfill_end_point.begin() != backfill_end_point.end());

    /* The end timestamp is the maximum of the timestamps we've seen. If you've
    been following closely (which you probably haven't because this is
    confusing) you should be thinking "How is that correct? If the region map
    says that we're at timestamp 900 on region A and 901 on region B, then that
    means we're missing the updates for the write with timestamp 900->901 on
    region A. If the region map has different timestamps for different regions,
    then it's not possible to boil down the current timestamp to a single
    number, right?" But this is wrong. Our backfill was, in fact, serialized
    with respect to all the writes. In fact, the reason why region A has
    timestamp 900 is that the write with timestamp 900->901 didn't touch any
    keys in region A. We didn't update the timestamp for region A because
    regions A and B were on separate threads, and we didn't want to send the
    operation to region A unnecessarily if we weren't actually updating any keys
    there. That's why it's OK to just take the maximum of all the timestamps
    that we see.
    TODO: If we change the way we shard such that each listener_t maps to a
    single B-tree on the same machine, then replace this loop with a strict
    assertion that requires everything to be at the same timestamp. */
    state_timestamp_t backfill_end_timestamp = backfill_end_point.begin()->second.earliest.timestamp;
    for (typename region_map_t<protocol_t, version_range_t>::const_iterator it = backfill_end_point.begin();
         it != backfill_end_point.end();
         ++it) {
        guarantee(it->second.is_coherent());
        guarantee(it->second.earliest.branch == branch_id_);
        backfill_end_timestamp = std::max(backfill_end_timestamp, it->second.earliest.timestamp);
    }

    guarantee(backfill_end_timestamp >= streaming_begin_point);

    current_timestamp_ = backfill_end_timestamp;
    write_queue_coro_pool_callback_.init(new boost_function_callback_t<write_queue_entry_t>(
            boost::bind(&listener_t<protocol_t>::perform_enqueued_write, this, _1, backfill_end_timestamp, _2)));
    write_queue_coro_pool_.init(new coro_pool_t<write_queue_entry_t>(
            WRITE_QUEUE_CORO_POOL_SIZE, &write_queue_, write_queue_coro_pool_callback_.get()));
    write_queue_semaphore_.set_capacity(WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY);

    if (write_queue_.size() <= WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY) {
        write_queue_has_drained_.pulse_if_not_already_pulsed();
    }

    wait_interruptible(&write_queue_has_drained_, interruptor);
}


template <class protocol_t>
listener_t<protocol_t>::listener_t(const base_path_t &base_path,
                                   io_backender_t *io_backender,
                                   mailbox_manager_t *mm,
                                   clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster_metadata,
                                   branch_history_manager_t<protocol_t> *branch_history_manager,
                                   broadcaster_t<protocol_t> *broadcaster,
                                   perfmon_collection_t *backfill_stats_parent,
                                   signal_t *interruptor,
                                   DEBUG_VAR order_source_t *order_source) THROWS_ONLY(interrupted_exc_t) :
    mailbox_manager_(mm),
    svs_(broadcaster->release_bootstrap_svs_for_listener()),
    branch_id_(broadcaster->get_branch_id()),
    uuid_(generate_uuid()),
    perfmon_collection_(),
    perfmon_collection_membership_(backfill_stats_parent, &perfmon_collection_, "backfill-serialization-" + uuid_to_str(uuid_)),
    /* TODO: Put the file in the data directory, not here */
    write_queue_(io_backender, serializer_filepath_t(base_path, "backfill-serialization-" + uuid_to_str(uuid_)), &perfmon_collection_),
    write_queue_semaphore_(WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY,
        WRITE_QUEUE_SEMAPHORE_TRICKLE_FRACTION),
    enforce_max_outstanding_writes_from_broadcaster_(MAX_OUTSTANDING_WRITES_FROM_BROADCASTER),
    write_mailbox_(mailbox_manager_,
        boost::bind(&listener_t::on_write, this, _1, _2, _3, _4, _5)),
    writeread_mailbox_(mailbox_manager_,
        boost::bind(&listener_t::on_writeread, this, _1, _2, _3, _4, _5, _6)),
    read_mailbox_(mailbox_manager_,
        boost::bind(&listener_t::on_read, this, _1, _2, _3, _4, _5))
{
    branch_birth_certificate_t<protocol_t> this_branch_history;
    {
        on_thread_t th(branch_history_manager->home_thread());
        this_branch_history = branch_history_manager->get_branch(branch_id_);
    }

    our_branch_region_ = this_branch_history.region;

#ifndef NDEBUG
    /* Confirm that `broadcaster_metadata` corresponds to `broadcaster` */
    boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > business_card =
        broadcaster_metadata->get();
    rassert(business_card && business_card.get());
    rassert(business_card.get().get().branch_id == broadcaster->get_branch_id());

    /* Make sure the initial state of the store is sane. Note that we assume
    that we're using the same `branch_history_manager_t` as the broadcaster, so
    an entry should already be present for the branch we're trying to join, and
    we skip calling `import_branch_history()`. */
    rassert(svs_->get_region() == this_branch_history.region);

    /* Snapshot the metainfo before we start receiving writes */
    object_buffer_t<fifo_enforcer_sink_t::exit_read_t> read_token;
    svs_->new_read_token(&read_token);
    region_map_t<protocol_t, binary_blob_t> initial_metainfo_blob;
    svs_->do_get_metainfo(order_source->check_in("listener_t(C)").with_read_mode(), &read_token, interruptor, &initial_metainfo_blob);
    region_map_t<protocol_t, version_range_t> initial_metainfo = to_version_range_map(initial_metainfo_blob);
#endif

    /* Attempt to register for writes */
    try_start_receiving_writes(broadcaster_metadata, interruptor);

    listener_intro_t<protocol_t> listener_intro;
    bool registration_is_done = registration_done_cond_.try_get_value(&listener_intro);
    guarantee(registration_is_done);

#ifndef NDEBUG
    region_map_t<protocol_t, version_range_t> expected_initial_metainfo(svs_->get_region(),
                                                                        version_range_t(version_t(branch_id_,
                                                                                                  listener_intro.broadcaster_begin_timestamp)));

    rassert(expected_initial_metainfo == initial_metainfo);
#endif

    /* Start streaming, just like we do after we finish a backfill */
    current_timestamp_ = listener_intro.broadcaster_begin_timestamp;
    write_queue_coro_pool_callback_.init(new boost_function_callback_t<write_queue_entry_t>(
            boost::bind(&listener_t<protocol_t>::perform_enqueued_write, this, _1, current_timestamp_, _2)));
    write_queue_coro_pool_.init(
        new coro_pool_t<write_queue_entry_t>(
            WRITE_QUEUE_CORO_POOL_SIZE, &write_queue_, write_queue_coro_pool_callback_.get()));
}

template <class protocol_t>
listener_t<protocol_t>::~listener_t() { }

template <class protocol_t>
signal_t *listener_t<protocol_t>::get_broadcaster_lost_signal() {
    return registrant_->get_failed_signal();
}

template <class protocol_t>
boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > >
listener_t<protocol_t>::get_backfiller_from_replier_bcard(const boost::optional<boost::optional<replier_business_card_t<protocol_t> > > &replier_bcard) {
    if (!replier_bcard) {
        return boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > >();
    } else if (!replier_bcard.get()) {
        return boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > >(
            boost::optional<backfiller_business_card_t<protocol_t> >());
    } else {
        return boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > >(
            boost::optional<backfiller_business_card_t<protocol_t> >(replier_bcard.get().get().backfiller_bcard));
    }
}

template <class protocol_t>
boost::optional<boost::optional<registrar_business_card_t<listener_business_card_t<protocol_t> > > >
listener_t<protocol_t>::get_registrar_from_broadcaster_bcard(const boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > &broadcaster_bcard) {
    if (!broadcaster_bcard) {
        return boost::optional<boost::optional<registrar_business_card_t<listener_business_card_t<protocol_t> > > >();
    } else if (!broadcaster_bcard.get()) {
        return boost::optional<boost::optional<registrar_business_card_t<listener_business_card_t<protocol_t> > > >(
            boost::optional<registrar_business_card_t<listener_business_card_t<protocol_t> > >());
    } else {
        return boost::optional<boost::optional<registrar_business_card_t<listener_business_card_t<protocol_t> > > >(
            boost::optional<registrar_business_card_t<listener_business_card_t<protocol_t> > >(broadcaster_bcard.get().get().registrar));
    }
}


/* `listener_intro_t` represents the introduction we expect to get from the
   broadcaster if all goes well. */
template <class protocol_t>
class intro_receiver_t : public signal_t {
public:
    listener_intro_t<protocol_t> intro;
    void fill(listener_intro_t<protocol_t> _intro) {
        guarantee(!is_pulsed());
        intro = _intro;
        pulse();
    }
};

template <class protocol_t>
void listener_t<protocol_t>::try_start_receiving_writes(
        clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, broadcaster_lost_exc_t)
{
    intro_receiver_t<protocol_t> intro_receiver;
    typename listener_business_card_t<protocol_t>::intro_mailbox_t
        intro_mailbox(mailbox_manager_,
                      boost::bind(&intro_receiver_t<protocol_t>::fill, &intro_receiver, _1));

    try {
        registrant_.init(new registrant_t<listener_business_card_t<protocol_t> >(
            mailbox_manager_,
            broadcaster->subview(&listener_t<protocol_t>::get_registrar_from_broadcaster_bcard),
            listener_business_card_t<protocol_t>(intro_mailbox.get_address(), write_mailbox_.get_address())));
    } catch (const resource_lost_exc_t &) {
        throw broadcaster_lost_exc_t();
    }

    wait_any_t waiter(&intro_receiver, registrant_->get_failed_signal());
    wait_interruptible(&waiter, interruptor);   /* May throw `interrupted_exc_t` */

    if (registrant_->get_failed_signal()->is_pulsed()) {
        throw broadcaster_lost_exc_t();
    } else {
        guarantee(intro_receiver.is_pulsed());
        registration_done_cond_.pulse(intro_receiver.intro);
    }
}

template <class protocol_t>
void listener_t<protocol_t>::on_write(const typename protocol_t::write_t &write,
        transition_timestamp_t transition_timestamp,
        order_token_t order_token,
        fifo_enforcer_write_token_t fifo_token,
        mailbox_addr_t<void()> ack_addr) THROWS_NOTHING {
    rassert(region_is_superset(our_branch_region_, write.get_region()));
    rassert(!region_is_empty(write.get_region()));
    order_token.assert_write_mode();

    coro_t::spawn_sometime(boost::bind(
        &listener_t<protocol_t>::enqueue_write, this,
        write, transition_timestamp, order_token, fifo_token, ack_addr,
        auto_drainer_t::lock_t(&drainer_)));
}

template <class protocol_t>
void listener_t<protocol_t>::enqueue_write(const typename protocol_t::write_t &write,
        transition_timestamp_t transition_timestamp,
        order_token_t order_token,
        fifo_enforcer_write_token_t fifo_token,
        mailbox_addr_t<void()> ack_addr,
        auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    try {
        /* Make sure that the broadcaster isn't sending us too many concurrent
        writes */
        semaphore_assertion_t::acq_t sem_acq(&enforce_max_outstanding_writes_from_broadcaster_);

        fifo_enforcer_sink_t::exit_write_t fifo_exit(&write_queue_entrance_sink_, fifo_token);
        wait_interruptible(&fifo_exit, keepalive.get_drain_signal());
        write_queue_semaphore_.co_lock_interruptible(keepalive.get_drain_signal());
        write_queue_.push(write_queue_entry_t(write, transition_timestamp, order_token, fifo_token));

        /* Release the semaphore before sending the response, because the
        broadcaster can send us a new write as soon as we send the ack */
        sem_acq.reset();
        send(mailbox_manager_, ack_addr);

    } catch (const interrupted_exc_t &) {
        /* pass */
    }
}

template <class protocol_t>
void listener_t<protocol_t>::perform_enqueued_write(const write_queue_entry_t &qe,
        state_timestamp_t backfill_end_timestamp,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    write_queue_semaphore_.unlock();
    if (write_queue_.size() <= WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY) {
        write_queue_has_drained_.pulse_if_not_already_pulsed();
    }

    write_token_pair_t write_token_pair;
    {
        fifo_enforcer_sink_t::exit_write_t fifo_exit(&store_entrance_sink_, qe.fifo_token);
        if (qe.transition_timestamp.timestamp_before() < backfill_end_timestamp) {
            return;
        }
        wait_interruptible(&fifo_exit, interruptor);
        advance_current_timestamp_and_pulse_waiters(qe.transition_timestamp);
        svs_->new_write_token_pair(&write_token_pair);
    }

#ifndef NDEBUG
        version_leq_metainfo_checker_callback_t<protocol_t> metainfo_checker_callback(qe.transition_timestamp.timestamp_before());
        metainfo_checker_t<protocol_t> metainfo_checker(&metainfo_checker_callback, svs_->get_region());
#endif

    typename protocol_t::write_response_t response;

    rassert(region_is_superset(svs_->get_region(), qe.write.get_region()));

    // This isn't used for client writes, so we don't want to wait for a disk ack.
    svs_->write(
        DEBUG_ONLY(metainfo_checker, )
        region_map_t<protocol_t, binary_blob_t>(svs_->get_region(),
            binary_blob_t(version_range_t(version_t(branch_id_, qe.transition_timestamp.timestamp_after())))),
        qe.write,
        &response,
        WRITE_DURABILITY_SOFT,
        qe.transition_timestamp,
        qe.order_token,
        &write_token_pair,
        interruptor);
}

template <class protocol_t>
void listener_t<protocol_t>::on_writeread(const typename protocol_t::write_t &write,
        transition_timestamp_t transition_timestamp,
        order_token_t order_token,
        fifo_enforcer_write_token_t fifo_token,
        mailbox_addr_t<void(typename protocol_t::write_response_t)> ack_addr,
        write_durability_t durability) THROWS_NOTHING {
    rassert(region_is_superset(our_branch_region_, write.get_region()));
    rassert(!region_is_empty(write.get_region()));
    rassert(region_is_superset(svs_->get_region(), write.get_region()));
    order_token.assert_write_mode();

    coro_t::spawn_sometime(boost::bind(
        &listener_t<protocol_t>::perform_writeread, this,
        write, transition_timestamp, order_token, fifo_token, ack_addr, durability,
        auto_drainer_t::lock_t(&drainer_)));
}

template <class protocol_t>
void listener_t<protocol_t>::perform_writeread(const typename protocol_t::write_t &write,
        transition_timestamp_t transition_timestamp,
        order_token_t order_token,
        fifo_enforcer_write_token_t fifo_token,
        mailbox_addr_t<void(typename protocol_t::write_response_t)> ack_addr,
        const write_durability_t durability,
        auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    try {
        /* Make sure the broadcaster isn't sending us too many writes */
        semaphore_assertion_t::acq_t sem_acq(&enforce_max_outstanding_writes_from_broadcaster_);

        write_token_pair_t write_token_pair;
        {
            {
                /* Briefly pass through `write_queue_entrance_sink_` in case we
                are receiving a mix of writes and write-reads */
                fifo_enforcer_sink_t::exit_write_t fifo_exit_1(&write_queue_entrance_sink_, fifo_token);
            }

            fifo_enforcer_sink_t::exit_write_t fifo_exit_2(&store_entrance_sink_, fifo_token);
            wait_interruptible(&fifo_exit_2, keepalive.get_drain_signal());

            advance_current_timestamp_and_pulse_waiters(transition_timestamp);

            svs_->new_write_token_pair(&write_token_pair);
        }

        // Make sure we can serve the entire operation without masking it.
        // (We shouldn't have been signed up for writereads if we couldn't.)
        rassert(region_is_superset(svs_->get_region(), write.get_region()));


#ifndef NDEBUG
        version_leq_metainfo_checker_callback_t<protocol_t> metainfo_checker_callback(transition_timestamp.timestamp_before());
        metainfo_checker_t<protocol_t> metainfo_checker(&metainfo_checker_callback, svs_->get_region());
#endif

        // Perform the operation
        typename protocol_t::write_response_t response;

        svs_->write(DEBUG_ONLY(metainfo_checker, )
                    region_map_t<protocol_t, binary_blob_t>(svs_->get_region(),
                                                            binary_blob_t(version_range_t(version_t(branch_id_, transition_timestamp.timestamp_after())))),
                    write,
                    &response,
                    durability,
                    transition_timestamp,
                    order_token,
                    &write_token_pair,
                    keepalive.get_drain_signal());

        /* Release the semaphore before sending the response, because the
        broadcaster can send us a new write as soon as we send the ack */
        sem_acq.reset();
        send(mailbox_manager_, ack_addr, response);

    } catch (const interrupted_exc_t &) {
        /* pass */
    }
}

template <class protocol_t>
void listener_t<protocol_t>::on_read(const typename protocol_t::read_t &read,
        state_timestamp_t expected_timestamp,
        order_token_t order_token,
        fifo_enforcer_read_token_t fifo_token,
        mailbox_addr_t<void(typename protocol_t::read_response_t)> ack_addr)
        THROWS_NOTHING {
    rassert(region_is_superset(our_branch_region_, read.get_region()));
    rassert(!region_is_empty(read.get_region()));
    rassert(region_is_superset(svs_->get_region(), read.get_region()));
    order_token.assert_read_mode();

    coro_t::spawn_sometime(boost::bind(
        &listener_t<protocol_t>::perform_read, this,
        read, expected_timestamp, order_token, fifo_token, ack_addr,
        auto_drainer_t::lock_t(&drainer_)));
}

template <class protocol_t>
void listener_t<protocol_t>::perform_read(const typename protocol_t::read_t &read,
        state_timestamp_t expected_timestamp,
        order_token_t order_token,
        fifo_enforcer_read_token_t fifo_token,
        mailbox_addr_t<void(typename protocol_t::read_response_t)> ack_addr,
        auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    try {
        read_token_pair_t read_token_pair;
        {
            {
                /* Briefly pass through `write_queue_entrance_sink_` in case we
                are receiving a mix of writes and write-reads */
                fifo_enforcer_sink_t::exit_read_t fifo_exit_1(
                    &write_queue_entrance_sink_, fifo_token);
            }

            fifo_enforcer_sink_t::exit_read_t fifo_exit_2(
                &store_entrance_sink_, fifo_token);
            wait_interruptible(&fifo_exit_2, keepalive.get_drain_signal());

            guarantee(current_timestamp_ == expected_timestamp);

            svs_->new_read_token_pair(&read_token_pair);
        }

#ifndef NDEBUG
        version_leq_metainfo_checker_callback_t<protocol_t> metainfo_checker_callback(
            expected_timestamp);
        metainfo_checker_t<protocol_t> metainfo_checker(
            &metainfo_checker_callback, svs_->get_region());
#endif

        // Perform the operation
        typename protocol_t::read_response_t response;
        svs_->read(
            DEBUG_ONLY(metainfo_checker, )
            read,
            &response,
            order_token,
            &read_token_pair,
            keepalive.get_drain_signal());

        send(mailbox_manager_, ack_addr, response);
    } catch (const interrupted_exc_t &) {
        /* pass */
    }
}

template <class protocol_t>
void listener_t<protocol_t>::wait_for_version(state_timestamp_t timestamp, signal_t *interruptor) {
    if (timestamp > current_timestamp_) {
        cond_t c;
        multimap_insertion_sentry_t<state_timestamp_t, cond_t *> sentry(&synchronize_waiters_, timestamp, &c);
        wait_interruptible(&c, interruptor);
    }
}

template <class protocol_t>
void listener_t<protocol_t>::advance_current_timestamp_and_pulse_waiters(transition_timestamp_t timestamp) {
    guarantee(timestamp.timestamp_before() == current_timestamp_);
    current_timestamp_ = timestamp.timestamp_after();

    for (std::multimap<state_timestamp_t, cond_t *>::const_iterator it = synchronize_waiters_.begin();
         it != synchronize_waiters_.upper_bound(current_timestamp_);
         ++it) {
        if (it->first < current_timestamp_) {
            // This cond should have already been pulsed because we assume timestamps move in discrete minimal steps.
            guarantee(it->second->is_pulsed());
        } else {
            // TODO: What if something's waiting eagerly?
            it->second->pulse();
        }
    }
}


#include "mock/dummy_protocol.hpp"
#include "memcached/protocol.hpp"
#include "rdb_protocol/protocol.hpp"

template class listener_t<memcached_protocol_t>;
template class listener_t<mock::dummy_protocol_t>;
template class listener_t<rdb_protocol_t>;
