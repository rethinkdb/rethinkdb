#include "clustering/immediate_consistency/branch/listener.hpp"

#include "clustering/immediate_consistency/branch/history.hpp"
#include "mock/dummy_protocol.hpp"
#include "memcached/protocol.hpp"


template <class protocol_t>
listener_t<protocol_t>::listener_t(mailbox_manager_t *mm,
				   clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster_metadata,
				   boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > bh,
				   store_view_t<protocol_t> *s,
				   clone_ptr_t<watchable_t<boost::optional<boost::optional<replier_business_card_t<protocol_t> > > > > replier,
				   backfill_session_id_t backfill_session_id,
				   signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, backfiller_lost_exc_t, broadcaster_lost_exc_t) :

    mailbox_manager(mm),
    branch_history(bh),
    store(s),

    write_mailbox(mailbox_manager, boost::bind(&listener_t::on_write, this,
					       auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),
    writeread_mailbox(mailbox_manager, boost::bind(&listener_t::on_writeread, this,
						   auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),
    read_mailbox(mailbox_manager, boost::bind(&listener_t::on_read, this,
					      auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4))
{
    if (interruptor->is_pulsed()) {
	throw interrupted_exc_t();
    }

    boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > business_card =
	broadcaster_metadata->get();
    if (business_card && business_card.get()) {
	branch_id = business_card.get().get().branch_id;
    } else {
	throw broadcaster_lost_exc_t();
    }

#ifndef NDEBUG
    branch_birth_certificate_t<protocol_t> this_branch_history =
	branch_history->get().branches[branch_id];

    rassert(region_is_superset(this_branch_history.region, store->get_region()));

    /* Sanity-check to make sure we're on the same timeline as the thing
       we're trying to join. The backfiller will perform an equivalent check,
       but if there's an error it would be nice to catch it where the action
       was initiated. */
    boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> read_token;
    store->new_read_token(read_token);

    typedef region_map_t<protocol_t, version_range_t> version_map_t;
    version_map_t start_point =
	region_map_transform<protocol_t, binary_blob_t, version_range_t>(
									 store->get_metainfo(read_token, interruptor),
									 &binary_blob_t::get<version_range_t>
									 );
    for (typename version_map_t::const_iterator it = start_point.begin();
	 it != start_point.end();
	 it++) {

	version_t version = it->second.latest;
	rassert(
                version.branch == branch_id ||
                version_is_ancestor(
				    branch_history->get(),
				    version,
				    version_t(branch_id, this_branch_history.initial_timestamp),
				    it->first)
                );
    }
#endif

    /* Attempt to register for reads and writes */
    try_start_receiving_writes(broadcaster_metadata, interruptor);
    rassert(registration_done_cond.get_ready_signal()->is_pulsed());

    state_timestamp_t streaming_begin_point =
	registration_done_cond.get_value().broadcaster_begin_timestamp;

    try {
	/* Go through a little song and dance to make sure that the
	 * backfiller will at least get us to the point that we will being
	 * live streaming from. */

	cond_t backfiller_is_up_to_date;
	mailbox_t<void()> ack_mbox(mailbox_manager, boost::bind(&cond_t::pulse, &backfiller_is_up_to_date));

	resource_access_t<replier_business_card_t<protocol_t> > replier_access(replier);
	send(mailbox_manager, replier_access.access().synchronize_mailbox, streaming_begin_point, ack_mbox.get_address());

	wait_any_t interruptor2(interruptor, replier_access.get_failed_signal());
	wait_interruptible(&backfiller_is_up_to_date, &interruptor2);

	/* Backfill */
	backfillee<protocol_t>(mailbox_manager,
			       branch_history,
			       store,
			       store->get_region(),
			       replier->subview(&listener_t<protocol_t>::get_backfiller_from_replier_bcard),
			       backfill_session_id,
			       interruptor
			       );
    } catch (resource_lost_exc_t) {
	throw backfiller_lost_exc_t();
    }

    boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> read_token2;
    store->new_read_token(read_token2);

    typedef region_map_t<protocol_t, version_range_t> version_map_t;

    version_map_t backfill_end_point =
	region_map_transform<protocol_t, binary_blob_t, version_range_t>(store->get_metainfo(read_token2, interruptor),
									 &binary_blob_t::get<version_range_t>
									 );

    /* Sanity checking. */

    /* Make sure the region is not empty. */
    rassert(backfill_end_point.begin() != backfill_end_point.end());

    state_timestamp_t backfill_end_timestamp = backfill_end_point.begin()->second.earliest.timestamp;

    /* Make sure the backfiller put us in a coherent position on the right
     * branch. */
#ifndef NDEBUG
    version_map_t expected_backfill_endpoint(store->get_region(),
					     version_range_t(version_t(branch_id, backfill_end_timestamp)));
#endif

    rassert(backfill_end_point == expected_backfill_endpoint);

    rassert(backfill_end_timestamp >= streaming_begin_point);

    current_timestamp = backfill_end_timestamp;
    backfill_done_cond.pulse(backfill_end_timestamp);
}


template <class protocol_t>
listener_t<protocol_t>::listener_t(mailbox_manager_t *mm,
				   clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster_metadata,
				   boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > bh,
				   broadcaster_t<protocol_t> *broadcaster,
				   signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) :

    mailbox_manager(mm),
    branch_history(bh),

    write_mailbox(mailbox_manager, boost::bind(&listener_t::on_write, this,
					       auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),
    writeread_mailbox(mailbox_manager, boost::bind(&listener_t::on_writeread, this,
						   auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),
    read_mailbox(mailbox_manager, boost::bind(&listener_t::on_read, this,
					      auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4))
{
    if (interruptor->is_pulsed()) {
	throw interrupted_exc_t();
    }

    /* We take our store directly from the broadcaster to make sure that we
       get the correct one. */
    rassert(broadcaster->bootstrap_store != NULL);
    store = broadcaster->bootstrap_store;
    broadcaster->bootstrap_store = NULL;

    branch_id = broadcaster->branch_id;

#ifndef NDEBUG
    /* Confirm that `broadcaster_metadata` corresponds to `broadcaster` */
    boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > business_card =
	broadcaster_metadata->get();
    rassert(business_card && business_card.get());
    rassert(business_card.get().get().branch_id == broadcaster->branch_id);

    /* Make sure the initial state of the store is sane */
    branch_birth_certificate_t<protocol_t> this_branch_history =
	branch_history->get().branches[branch_id];
    rassert(store->get_region() == this_branch_history.region);
    /* Snapshot the metainfo before we start receiving writes */
    boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> read_token;
    store->new_read_token(read_token);

    region_map_t<protocol_t, binary_blob_t> initial_metainfo =
	store->get_metainfo(read_token, interruptor);
#endif

    /* Attempt to register for reads and writes */
    try_start_receiving_writes(broadcaster_metadata, interruptor);
    rassert(registration_done_cond.get_ready_signal()->is_pulsed());

#ifndef NDEBUG
    region_map_t<protocol_t, binary_blob_t> expected_initial_metainfo(store->get_region(),
								      binary_blob_t(version_range_t(version_t(branch_id,
													      registration_done_cond.get_value().broadcaster_begin_timestamp))));

    rassert(expected_initial_metainfo == initial_metainfo);
#endif

    /* Pretend we just finished an imaginary backfill */
    current_timestamp = registration_done_cond.get_value().broadcaster_begin_timestamp;
    backfill_done_cond.pulse(registration_done_cond.get_value().broadcaster_begin_timestamp);
}

template <class protocol_t>
signal_t *listener_t<protocol_t>::get_broadcaster_lost_signal() {
    return registrant->get_failed_signal();
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

template <class protocol_t>
void listener_t<protocol_t>::try_start_receiving_writes(
	clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster,
	signal_t *interruptor)
	THROWS_ONLY(interrupted_exc_t, broadcaster_lost_exc_t)
{
    intro_receiver_t intro_receiver;
    typename listener_business_card_t<protocol_t>::intro_mailbox_t intro_mailbox(
	mailbox_manager, boost::bind(&intro_receiver_t::fill, &intro_receiver, _1, _2, _3));

    try {
	registrant.reset(new registrant_t<listener_business_card_t<protocol_t> >(
	    mailbox_manager,
	    broadcaster->subview(&listener_t<protocol_t>::get_registrar_from_broadcaster_bcard),
	    listener_business_card_t<protocol_t>(intro_mailbox.get_address(), write_mailbox.get_address())
	    ));
    } catch (resource_lost_exc_t) {
	throw broadcaster_lost_exc_t();
    }

    wait_any_t waiter(&intro_receiver, registrant->get_failed_signal());
    wait_interruptible(&waiter, interruptor);   /* May throw `interrupted_exc_t` */

    if (registrant->get_failed_signal()->is_pulsed()) {
	throw broadcaster_lost_exc_t();
    } else {
	rassert(intro_receiver.is_pulsed());
	registration_done_cond.pulse(intro_receiver.intro);
    }
}

template <class protocol_t>
void listener_t<protocol_t>::on_write(auto_drainer_t::lock_t keepalive,
	typename protocol_t::write_t write,
	transition_timestamp_t transition_timestamp,
	fifo_enforcer_write_token_t fifo_token,
	mailbox_addr_t<void()> ack_addr)
	THROWS_NOTHING {
    try {
	boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> token;
	{
	    /* Enforce that we start our transaction in the same order as we
	    entered the FIFO at the broadcaster. */
	    fifo_enforcer_sink_t::exit_write_t fifo_exit(&fifo_sink, fifo_token);
	    wait_interruptible(&fifo_exit, keepalive.get_drain_signal());

	    /* Validate write. */
	    rassert(region_is_superset(branch_history->get().branches[branch_id].region, write.get_region()));
	    rassert(!region_is_empty(write.get_region()));

	    /* Block until registration has completely succeeded or failed.
	    (May throw `interrupted_exc_t`) */
	    wait_interruptible(registration_done_cond.get_ready_signal(), keepalive.get_drain_signal());

	    /* Block until the backfill succeeds or fails. If the backfill
	    fails, then the constructor will throw an exception, so
	    `keepalive.get_drain_signal()` will be pulsed. */
	    wait_interruptible(backfill_done_cond.get_ready_signal(), keepalive.get_drain_signal());

	    if (transition_timestamp.timestamp_before() < backfill_done_cond.get_value()) {
		/* `write` is a duplicate; we got it both as part of the
		backfill, and from the broadcaster. Ignore this copy of it. */
		return;
	    }

	    advance_current_timestamp_and_pulse_waiters(transition_timestamp);

	    store->new_write_token(token);
	    /* Now that we've gotten a write token, it's safe to allow the next write or read to proceed. */
	}

	/* Mask out any parts of the operation that don't apply to us */
	write = write.shard(region_intersection(write.get_region(), store->get_region()));

	cond_t non_interruptor;
	store->write(
	    DEBUG_ONLY(
		region_map_t<protocol_t, binary_blob_t>(store->get_region(),
		    binary_blob_t(version_range_t(version_t(branch_id, transition_timestamp.timestamp_before())))),
		)
	    region_map_t<protocol_t, binary_blob_t>(store->get_region(),
		binary_blob_t(version_range_t(version_t(branch_id, transition_timestamp.timestamp_after())))),
	    write,
	    transition_timestamp,
	    token,
	    &non_interruptor);

	send(mailbox_manager, ack_addr);

    } catch (interrupted_exc_t) {
	return;
    }
}

template <class protocol_t>
void listener_t<protocol_t>::on_writeread(auto_drainer_t::lock_t keepalive,
	typename protocol_t::write_t write,
	transition_timestamp_t transition_timestamp,
	fifo_enforcer_write_token_t fifo_token,
	mailbox_addr_t<void(typename protocol_t::write_response_t)> ack_addr)
	THROWS_NOTHING
{
    try {
	boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> token;
	{
	    fifo_enforcer_sink_t::exit_write_t fifo_exit(&fifo_sink, fifo_token);
	    wait_interruptible(&fifo_exit, keepalive.get_drain_signal());

	    /* Validate write. */
	    rassert(region_is_superset(branch_history->get().branches[branch_id].region, write.get_region()));
	    rassert(!region_is_empty(write.get_region()));

	    /* We can't possibly be receiving writereads unless we successfully
	    registered and completed a backfill */
	    rassert(registration_done_cond.get_ready_signal()->is_pulsed());
	    rassert(backfill_done_cond.get_ready_signal()->is_pulsed());

	    /* This mustn't be a duplicate operation because we can't register for
	    writereads until the backfill is over */
	    rassert(transition_timestamp.timestamp_before() >=
		backfill_done_cond.get_value());

	    advance_current_timestamp_and_pulse_waiters(transition_timestamp);

	    store->new_write_token(token);
	    /* Now that we've gotten a write token, allow the next guy to proceed */
	}

	// Make sure we can serve the entire operation without masking it.
	// (We shouldn't have been signed up for writereads if we couldn't.)
	rassert(region_is_superset(store->get_region(), write.get_region()));

	// Perform the operation
	cond_t non_interruptor;
	typename protocol_t::write_response_t response = store->write(DEBUG_ONLY(
		region_map_t<protocol_t, binary_blob_t>(store->get_region(),
		    binary_blob_t(version_range_t(version_t(branch_id, transition_timestamp.timestamp_before())))),
		)
	    region_map_t<protocol_t, binary_blob_t>(store->get_region(),
		binary_blob_t(version_range_t(version_t(branch_id, transition_timestamp.timestamp_after())))),
	    write,
	    transition_timestamp,
	    token,
	    &non_interruptor);

	send(mailbox_manager, ack_addr, response);

    } catch (interrupted_exc_t) {
	return;
    }
}

template <class protocol_t>
void listener_t<protocol_t>::on_read(auto_drainer_t::lock_t keepalive,
	typename protocol_t::read_t read,
	DEBUG_ONLY_VAR state_timestamp_t expected_timestamp,
	fifo_enforcer_read_token_t fifo_token,
	mailbox_addr_t<void(typename protocol_t::read_response_t)> ack_addr)
	THROWS_NOTHING
{
    try {
	boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> token;
	{
	    fifo_enforcer_sink_t::exit_read_t fifo_exit(&fifo_sink, fifo_token);
	    wait_interruptible(&fifo_exit, keepalive.get_drain_signal());

	    /* Validate read. */
	    rassert(region_is_superset(branch_history->get().branches[branch_id].region, read.get_region()));
	    rassert(!region_is_empty(read.get_region()));

	    /* We can't possibly be receiving reads unless we successfully
	    registered and completed a backfill */
	    rassert(registration_done_cond.get_ready_signal()->is_pulsed());
	    rassert(backfill_done_cond.get_ready_signal()->is_pulsed());

	    /* Now that we have the superblock, allow the next guy to proceed */
	    store->new_read_token(token);
	}

	/* Make sure we can serve the entire operation without masking it.
	(We shouldn't have been signed up for reads if we couldn't.) */
	rassert(region_is_superset(store->get_region(), read.get_region()));

	/* Perform the operation */
	typename protocol_t::read_response_t response = store->read(
	    DEBUG_ONLY(
		region_map_t<protocol_t, binary_blob_t>(store->get_region(),
		    binary_blob_t(version_range_t(version_t(branch_id, expected_timestamp)))),
		)
	    read,
	    token,
	    keepalive.get_drain_signal());

	send(mailbox_manager, ack_addr, response);

    } catch (interrupted_exc_t) {
	return;
    }
}

template <class protocol_t>
void listener_t<protocol_t>::wait_for_version(state_timestamp_t timestamp, signal_t *interruptor) {
    rassert(backfill_done_cond.get_ready_signal()->is_pulsed(), "This shouldn't be called before the constructor has completed.");
    if (timestamp > current_timestamp) {
	cond_t c;
	multimap_insertion_sentry_t<state_timestamp_t, cond_t *> sentry(&synchronize_waiters, timestamp, &c);
	wait_interruptible(&c, interruptor);
    }
}

template <class protocol_t>
void listener_t<protocol_t>::advance_current_timestamp_and_pulse_waiters(transition_timestamp_t timestamp) {
    rassert(timestamp.timestamp_before() == current_timestamp);
    current_timestamp = timestamp.timestamp_after();

    for (std::multimap<state_timestamp_t, cond_t *>::const_iterator it  = synchronize_waiters.begin();
	 it != synchronize_waiters.upper_bound(current_timestamp);
	 ++it) {
	if (it->first < current_timestamp) {
	    rassert(it->second->is_pulsed(), "This cond should have already been pulsed because we assume timestamps move in discrete minimal steps.");
	} else {
	    it->second->pulse();
	}
    }
}



template class listener_t<memcached_protocol_t>;
template class listener_t<mock::dummy_protocol_t>;
