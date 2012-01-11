#ifndef __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_LISTENER_HPP__
#define __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_LISTENER_HPP__

#include "clustering/immediate_consistency/branch/backfillee.hpp"
#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "clustering/resource.hpp"
#include "clustering/registrant.hpp"
#include "concurrency/promise.hpp"
#include "protocol_api.hpp"
#include "rpc/directory/view.hpp"

/* Forward declarations (so we can friend them) */

template<class protocol_t> class broadcaster_t;
template<class protocol_t> class replier_t;

/* `listener_t` keeps a store-view in sync with a branch. Its constructor
backfills from an existing mirror on a branch into the store, and as long as it
exists the store will receive real-time updates.

There are three ways a `listener_t` can go wrong:
 *  You can interrupt the constructor. It will throw `interrupted_exc_t`. The
    store may be left in a half-backfilled state; you can determine this through
    the store's metadata.
 *  It can fail to join the branch. In this case, the constructor will throw a
    `resource_lost_exc_t`.
 *  It can successfully join the branch, but not be able to keep up with real-
    time updates. In that case, the constructor will succeed but
    `get_outdated_signal()` will be pulsed. Note that it's possible for
    `get_outdated_signal()` to be pulsed at the time the constructor returns.
*/

template<class protocol_t>
class listener_t {

public:
    listener_t(
            mailbox_manager_t *mm,
            clone_ptr_t<directory_rview_t<std::map<branch_id_t, broadcaster_business_card_t<protocol_t> > > > broadcaster_directory,
            boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > bh,
            store_view_t<protocol_t> *s,
            branch_id_t bid,
            clone_ptr_t<directory_single_rview_t<boost::optional<backfiller_business_card_t<protocol_t> > > > backfiller,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t) :

        mailbox_manager(mm),
        branch_history(bh),
        store(s),
        branch_id(bid),

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

#ifndef NDEBUG
        branch_birth_certificate_t<protocol_t> this_branch_history =
            branch_history->get().branches[branch_id];

        rassert(region_is_superset(this_branch_history.region, store->get_region()));

        /* Sanity-check to make sure we're on the same timeline as the thing
        we're trying to join. The backfiller will perform an equivalent check,
        but if there's an error it would be nice to catch it where the action
        was initiated. */
        std::vector<std::pair<typename protocol_t::region_t, version_range_t> > start_point_pairs =
            region_map_transform<protocol_t, binary_blob_t, version_range_t>(
                store->begin_read_transaction(interruptor)->get_metadata(interruptor),
                &binary_blob_t::get<version_range_t>
                ).get_as_pairs();
        for (int i = 0; i < (int)start_point_pairs.size(); i++) {
            version_t version = start_point_pairs[i].second.latest;
            rassert(
                version.branch == branch_id ||
                version_is_ancestor(
                    branch_history->get(),
                    version,
                    version_t(branch_id, this_branch_history.initial_timestamp),
                    start_point_pairs[i].first)
                );
        }
#endif

        /* Figure out which peer the broadcaster lives on. */
        peer_id_t broadcaster_peer;
        int num_matches_found = 0;
        {
            connectivity_service_t::peers_list_freeze_t freeze(
                broadcaster_directory->get_directory_service()->get_connectivity_service()
                );
            std::set<peer_id_t> peers = broadcaster_directory->
                get_directory_service()->get_connectivity_service()->
                get_peers_list();
            for (std::set<peer_id_t>::iterator it = peers.begin();
                    it != peers.end(); it++) {
                if (broadcaster_directory->get_value(*it).get().count(branch_id) != 0) {
                    broadcaster_peer = *it;
                    num_matches_found++;
                }
            }
        }
        rassert(num_matches_found <= 1);
        if (num_matches_found == 0) {
            /* We already lost contact with the broadcaster peer */
            registration_result_cond.pulse(boost::optional<intro_t>());
        } else {
            /* Attempt to register for reads and writes */
            try_start_receiving_writes(
                broadcaster_directory->
                    get_peer_view(broadcaster_peer)->
                    template subview<boost::optional<broadcaster_business_card_t<protocol_t> > >(
                        optional_member_lens<branch_id_t, broadcaster_business_card_t<protocol_t> >(branch_id)
                        ),
                interruptor);
        }

        /* Backfill */
        backfillee<protocol_t>(
            mailbox_manager,
            branch_history,
            store,
            backfiller,
            interruptor
            );

        /* Check where the backfill sent us to. If the backfill sent us to a
        reasonable place, then pulse `backfill_done_cond`. Otherwise, pulse
        `backfill_failed_cond`. One or the other must be done before we return
        from the constructor. */
        if (registration_result_cond.get_value()) {

            state_timestamp_t streaming_begin_point =
                registration_result_cond.get_value()->broadcaster_begin_timestamp;
            std::vector<std::pair<typename protocol_t::region_t, version_range_t> > backfill_end_point =
                region_map_transform<protocol_t, binary_blob_t, version_range_t>(
                    store->begin_read_transaction(interruptor)->get_metadata(interruptor),
                    &binary_blob_t::get<version_range_t>
                    ).get_as_pairs();

            /* Sanity checking. The backfiller should have put us into a
            coherent position because it shouldn't have been in the metadata if
            it was incoherent. */
            rassert(backfill_end_point.size() == 1);
            rassert(backfill_end_point[0].second.is_coherent());
            rassert(backfill_end_point[0].second.earliest.branch == branch_id);

            if (backfill_end_point[0].second.earliest.timestamp >= streaming_begin_point) {
                /* The backfill put us in a reasonable spot. Pulse
                `backfill_done_cond` so real-time operations can proceed. */
                backfill_done_cond.pulse(backfill_end_point[0].second.earliest.timestamp);
            } else {
                /* There was a gap between the data the backfiller sent us and
                the beginning of the data we got from the broadcaster. We're
                on the branch (namely, at the point the backfiller put us) but
                we can't keep up with realtime updates. Pulse
                `backfill_failed_cond` to notify realtime operations not to
                proceed and set `outdated_signal` so people know we're not
                keeping up with real-time operations. But don't throw an
                exception, since we're on the branch. */
                backfill_failed_cond.pulse();
                outdated_signal.add(&backfill_failed_cond);
            }
        }
    }

    /* Returns a signal that is pulsed if the mirror is not in contact with the
    master. */
    signal_t *get_outdated_signal() {
        return &outdated_signal;
    }

private:
    friend class broadcaster_t<protocol_t>;
    friend class replier_t<protocol_t>;

    /* `intro_t` represents the introduction we expect to get from the
    broadcaster if all goes well. */
    class intro_t {
    public:
        typename listener_business_card_t<protocol_t>::upgrade_mailbox_t::address_t upgrade_mailbox;
        typename listener_business_card_t<protocol_t>::downgrade_mailbox_t::address_t downgrade_mailbox;
        state_timestamp_t broadcaster_begin_timestamp;
    };

    /* Support class for `start_receiving_writes()`. It's defined out here, as
    opposed to locally in `start_receiving_writes()`, so that we can
    `boost::bind()` to it. */
    class intro_receiver_t : public signal_t {
    public:
        intro_t intro;
        void fill(state_timestamp_t its,
                typename listener_business_card_t<protocol_t>::upgrade_mailbox_t::address_t um,
                typename listener_business_card_t<protocol_t>::downgrade_mailbox_t::address_t dm) {
            rassert(!is_pulsed());
            intro.broadcaster_begin_timestamp = its;
            intro.upgrade_mailbox = um;
            intro.downgrade_mailbox = dm;
            pulse();
        }
    };

    /* This version of the `listener_t` constructor is called when we are
    becoming the first mirror of a new branch. It's private because it shouldn't
    be used for any other purpose. */
    listener_t(
            mailbox_manager_t *mm,
            clone_ptr_t<directory_rview_t<std::map<branch_id_t, broadcaster_business_card_t<protocol_t> > > > broadcaster_directory,
            boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > bh,
            store_view_t<protocol_t> *s,
            branch_id_t bid,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) :

        mailbox_manager(mm),
        branch_history(bh),
        store(s),
        branch_id(bid),

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

        region_map_t<protocol_t, binary_blob_t> initial_metadata =
            store->begin_read_transaction(interruptor)->get_metadata(interruptor);
        rassert(initial_metadata.get_as_pairs().size() == 1);
        version_range_t initial_version = binary_blob_t::get<version_range_t>(initial_metadata.get_as_pairs()[0].second);
        rassert(initial_version.is_coherent());
        rassert(initial_version.earliest.branch == branch_id);
        state_timestamp_t initial_timestamp = initial_version.earliest.timestamp;

#ifndef NDEBUG
        /* Make sure the initial state of the store is sane */
        branch_birth_certificate_t<protocol_t> this_branch_history =
            branch_history->get().branches[branch_id];
        rassert(store->get_region() == this_branch_history.region);
#endif

        /* Attempt to register for reads and writes */
        try_start_receiving_writes(
            broadcaster_directory->
                get_peer_view(mailbox_manager->get_connectivity_service()->get_me())->
                template subview<boost::optional<broadcaster_business_card_t<protocol_t> > >(
                    optional_member_lens<branch_id_t, broadcaster_business_card_t<protocol_t> >(branch_id)
                    ),
            interruptor);

        /* Make sure that `broadcast_begin_timestamp` matches the initial
        metadata */
        if (registration_result_cond.get_value()) {
            rassert(registration_result_cond.get_value()->broadcaster_begin_timestamp ==
                    initial_timestamp);
        }

        /* Pretend we just finished an imaginary backfill */
        backfill_done_cond.pulse(initial_timestamp);

        /* When operating in this mode, the only reason we can become outdated
        is if we lose contact with the broadcaster. There cannot possibly be a
        data gap, and `backfill_failed_cond` is unused. */
    }

    /* `try_start_receiving_writes()` is called from within the constructors. It
    tries to register with the master. It throws `interrupted_exc_t` if
    `interruptor` is pulsed. Otherwise, it fills `registration_result_cond` with
    a value indicating if the registration succeeded or not, and with the intro
    we got from the broadcaster if it succeeded. */
    void try_start_receiving_writes(
            clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<protocol_t> > > > broadcaster,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t)
    {
        intro_receiver_t intro_receiver;
        typename listener_business_card_t<protocol_t>::intro_mailbox_t intro_mailbox(
            mailbox_manager, boost::bind(&intro_receiver_t::fill, &intro_receiver, _1, _2, _3));

        try {
            registrant.reset(new registrant_t<listener_business_card_t<protocol_t> >(
                mailbox_manager,
                broadcaster->subview(
                    optional_monad_lens<
                            registrar_business_card_t<listener_business_card_t<protocol_t> >,
                            broadcaster_business_card_t<protocol_t> >(
                        field_lens(&broadcaster_business_card_t<protocol_t>::registrar)
                        )
                    ),
                listener_business_card_t<protocol_t>(intro_mailbox.get_address(), write_mailbox.get_address())
                ));
        } catch (resource_lost_exc_t) {
            registration_result_cond.pulse(boost::optional<intro_t>());
            return;
        }

        wait_any_t waiter(&intro_receiver, interruptor, registrant->get_failed_signal());
        waiter.wait_lazily_unordered();
        if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }

        if (registrant->get_failed_signal()->is_pulsed()) {
            registration_result_cond.pulse(boost::optional<intro_t>());
        } else {
            rassert(intro_receiver.is_pulsed());
            registration_result_cond.pulse(boost::optional<intro_t>(
                intro_receiver.intro));
        }

        /* If we lose contact with the master, mark ourselves outdated */
        outdated_signal.add(registrant->get_failed_signal());
    }

    void on_write(auto_drainer_t::lock_t keepalive,
            typename protocol_t::write_t write,
            transition_timestamp_t transition_timestamp,
            fifo_enforcer_write_token_t fifo_token,
            async_mailbox_t<void()>::address_t ack_addr)
            THROWS_NOTHING
    {
        try {
            /* Enforce that we start our transaction in the same order as we
            entered the FIFO at the broadcaster. */
            fifo_enforcer_sink_t::exit_write_t fifo_exit(&fifo_sink, fifo_token, keepalive.get_drain_signal());

            /* Validate write. */
            rassert(region_is_superset(branch_history->get().branches[branch_id].region, write.get_region()));
            rassert(!region_is_empty(write.get_region()));

            /* Block until registration has completely succeeded or failed. */
            {
                wait_any_t waiter(registration_result_cond.get_ready_signal(), keepalive.get_drain_signal());
                waiter.wait_lazily_unordered();
                if (keepalive.get_drain_signal()->is_pulsed()) {
                    throw interrupted_exc_t();
                }
            }

            if (!registration_result_cond.get_value()) {
                /* Registration never succeeded; for some reason a few writes
                were transmitted anyway, and we're one of them. Abort. */
                return;
            }

            /* Block until the backfill succeeds or fails. */
            {
                wait_any_t waiter(backfill_done_cond.get_ready_signal(), &backfill_failed_cond, keepalive.get_drain_signal());
                waiter.wait_lazily_unordered();
                if (keepalive.get_drain_signal()->is_pulsed()) {
                    throw interrupted_exc_t();
                }
            }

            if (backfill_failed_cond.is_pulsed()) {
                /* There was a gap between the end of the backfill and the
                beginning of the real-time streaming operations. The only way to
                stay consistent is to throw away all the real-time streaming
                operations and mark ourselves outdated. The constructor will
                take care of marking us outdated. */
                return;
            }

            if (transition_timestamp.timestamp_before() < backfill_done_cond.get_value()) {
                /* `write` is a duplicate; we got it both as part of the
                backfill, and from the broadcaster. Ignore this copy of it. */
                return;
            }

            boost::shared_ptr<typename store_view_t<protocol_t>::write_transaction_t> transaction =
                store->begin_write_transaction(keepalive.get_drain_signal());

            /* Now that we've started a transaction, the superblock is locked,
            so it's safe to allow the next write or read to proceed. */
            fifo_exit.reset();

#ifndef NDEBUG
            /* Sanity-check metadata */
            std::vector<std::pair<typename protocol_t::region_t, version_range_t> > backfill_end_point =
                region_map_transform<protocol_t, binary_blob_t, version_range_t>(
                    transaction->get_metadata(keepalive.get_drain_signal()),
                    &binary_blob_t::get<version_range_t>
                    ).get_as_pairs();
            rassert(backfill_end_point.size() == 1);
            rassert(backfill_end_point[0].second.is_coherent());
            rassert(backfill_end_point[0].second.earliest.timestamp ==
                transition_timestamp.timestamp_before());
#endif

            /* Update metadata */
            transaction->set_metadata(region_map_t<protocol_t, binary_blob_t>(
                store->get_region(),
                binary_blob_t(version_range_t(version_t(branch_id, transition_timestamp.timestamp_after())))
                ));

            /* Mask out any parts of the operation that don't apply to us */
            write = write.shard(region_intersection(write.get_region(), store->get_region()));

            /* Perform the operation */
            transaction->write(
                write,
                transition_timestamp);

            send(mailbox_manager, ack_addr);

        } catch (interrupted_exc_t) {
            return;
        }
    }

    /* See the note at the place where `writeread_mailbox` is declared for an
    explanation of why `on_writeread()` and `on_read()` are here. */

    void on_writeread(auto_drainer_t::lock_t keepalive,
            typename protocol_t::write_t write,
            transition_timestamp_t transition_timestamp,
            fifo_enforcer_write_token_t fifo_token,
            typename async_mailbox_t<void(typename protocol_t::write_response_t)>::address_t ack_addr)
            THROWS_NOTHING
    {
        try {
            fifo_enforcer_sink_t::exit_write_t fifo_exit(&fifo_sink, fifo_token, keepalive.get_drain_signal());

            /* Validate write. */
            rassert(region_is_superset(branch_history->get().branches[branch_id].region, write.get_region()));
            rassert(!region_is_empty(write.get_region()));

            /* We can't possibly be receiving writereads unless we successfully
            registered and completed a backfill */
            rassert(registration_result_cond.get_ready_signal()->is_pulsed() &&
                registration_result_cond.get_value());
            rassert(backfill_done_cond.get_ready_signal()->is_pulsed());

            /* This mustn't be a duplicate operation because we can't register for
            writereads until the backfill is over */
            rassert(transition_timestamp.timestamp_before() >=
                backfill_done_cond.get_value());

            boost::shared_ptr<typename store_view_t<protocol_t>::write_transaction_t> transaction =
                store->begin_write_transaction(keepalive.get_drain_signal());

            /* Now that we have the superblock, allow the next guy to proceed */
            fifo_exit.reset();

#ifndef NDEBUG
            /* Sanity-check metadata */
            std::vector<std::pair<typename protocol_t::region_t, version_range_t> > backfill_end_point =
                region_map_transform<protocol_t, binary_blob_t, version_range_t>(
                    transaction->get_metadata(keepalive.get_drain_signal()),
                    &binary_blob_t::get<version_range_t>
                    ).get_as_pairs();
            rassert(backfill_end_point.size() == 1);
            rassert(backfill_end_point[0].second.is_coherent());
            rassert(backfill_end_point[0].second.earliest.timestamp ==
                transition_timestamp.timestamp_before());
#endif

            /* Update metadata */
            transaction->set_metadata(region_map_t<protocol_t, binary_blob_t>(
                store->get_region(),
                binary_blob_t(version_range_t(version_t(branch_id, transition_timestamp.timestamp_after())))
                ));

            /* Make sure we can serve the entire operation without masking it.
            (We shouldn't have been signed up for writereads if we couldn't.) */
            rassert(region_is_superset(store->get_region(), write.get_region()));

            /* Perform the operation */
            typename protocol_t::write_response_t response = transaction->write(
                write,
                transition_timestamp);

            send(mailbox_manager, ack_addr, response);

        } catch (interrupted_exc_t) {
            return;
        }
    }

    void on_read(auto_drainer_t::lock_t keepalive,
            typename protocol_t::read_t read,
            UNUSED state_timestamp_t expected_timestamp,
            fifo_enforcer_read_token_t fifo_token,
            typename async_mailbox_t<void(typename protocol_t::read_response_t)>::address_t ack_addr)
            THROWS_NOTHING
    {
        try {
            fifo_enforcer_sink_t::exit_read_t fifo_exit(&fifo_sink, fifo_token, keepalive.get_drain_signal());

            /* Validate read. */
            rassert(region_is_superset(branch_history->get().branches[branch_id].region, read.get_region()));
            rassert(!region_is_empty(read.get_region()));

            /* We can't possibly be receiving reads unless we successfully
            registered and completed a backfill */
            rassert(registration_result_cond.get_ready_signal()->is_pulsed() &&
                registration_result_cond.get_value());
            rassert(backfill_done_cond.get_ready_signal()->is_pulsed());

            boost::shared_ptr<typename store_view_t<protocol_t>::read_transaction_t> transaction =
                store->begin_read_transaction(keepalive.get_drain_signal());

            /* Now that we have the superblock, allow the next guy to proceed */
            fifo_exit.reset();

#ifndef NDEBUG
            /* Sanity-check metadata */
            std::vector<std::pair<typename protocol_t::region_t, version_range_t> > backfill_end_point =
                region_map_transform<protocol_t, binary_blob_t, version_range_t>(
                    transaction->get_metadata(keepalive.get_drain_signal()),
                    &binary_blob_t::get<version_range_t>
                    ).get_as_pairs();
            rassert(backfill_end_point.size() == 1);
            rassert(backfill_end_point[0].second.is_coherent());
            rassert(backfill_end_point[0].second.earliest.timestamp ==
                expected_timestamp);
#endif

            /* Make sure we can serve the entire operation without masking it.
            (We shouldn't have been signed up for reads if we couldn't.) */
            rassert(region_is_superset(store->get_region(), read.get_region()));

            /* Perform the operation */
            typename protocol_t::read_response_t response = transaction->read(
                read,
                keepalive.get_drain_signal());

            send(mailbox_manager, ack_addr, response);

        } catch (interrupted_exc_t) {
            return;
        }
    }

    mailbox_manager_t *mailbox_manager;

    /* This variable looks useless, since it's set in the constructor but not
    used by any of the methods, but in reality it's important because it's used
    by `replier_t` which is our friend. */
    boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > branch_history;

    store_view_t<protocol_t> *store;

    branch_id_t branch_id;

    /* `upgrade_mailbox` and `broadcaster_begin_timestamp` are valid only if we
    successfully registered with the broadcaster at some point. As a sanity
    check, we put them in a `promise_t` that only gets pulsed when we
    successfully register. If registration fails, we pulse
    `registration_result_cond` with an empty `boost::optional`. */
    promise_t<boost::optional<intro_t> > registration_result_cond;

    /* If the backfill succeeds, `backfill_done_cond` will be pulsed with the
    point that the backfill brought us to. If the backfill fails,
    `backfill_failed_cond` will be pulsed. */
    promise_t<state_timestamp_t> backfill_done_cond;
    cond_t backfill_failed_cond;

    fifo_enforcer_sink_t fifo_sink;

    auto_drainer_t drainer;

    typename listener_business_card_t<protocol_t>::write_mailbox_t write_mailbox;

    /* `writeread_mailbox` and `read_mailbox` live on the `listener_t` even
    though they don't get used until the `replier_t` is constructed. The reason
    `writeread_mailbox` has to live here is so we don't drop writes if the
    `replier_t` is destroyed without doing a warm shutdown but the `listener_t`
    stays alive. The reason `read_mailbox` is here is for consistency, and to
    have all the query-handling code in one place. */
    typename listener_business_card_t<protocol_t>::writeread_mailbox_t writeread_mailbox;
    typename listener_business_card_t<protocol_t>::read_mailbox_t read_mailbox;

    boost::scoped_ptr<registrant_t<listener_business_card_t<protocol_t> > > registrant;

    /* `outdated_signal` is pulsed if we become outdated for any reason. */
    wait_any_t outdated_signal;
};

#endif /* __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_LISTENER_HPP__ */
