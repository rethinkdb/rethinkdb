// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/reactor/reactor.hpp"

#include <exception>
#include <vector>

#include "errors.hpp"
#include <boost/ptr_container/ptr_vector.hpp>

#include "clustering/administration/http/json_adapters.hpp"
#include "clustering/immediate_consistency/branch/backfillee.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/direct_reader.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "config/args.hpp"
#include "stl_utils.hpp"

template <class protocol_t>
reactor_t<protocol_t>::backfill_candidate_t::backfill_candidate_t(version_range_t _version_range, std::vector<backfill_location_t> _places_to_get_this_version, bool _present_in_our_store)
    : version_range(_version_range), places_to_get_this_version(_places_to_get_this_version),
      present_in_our_store(_present_in_our_store)
{ }

class divergent_data_exc_t : public std::exception {
    const char *what() const throw() {
        return "Divergent data was found, catch this and grind everything to a halt\n";
    }
};

template <class protocol_t>
void reactor_t<protocol_t>::update_best_backfiller(const region_map_t<protocol_t, version_range_t> &offered_backfill_versions,
                                                   const typename backfill_candidate_t::backfill_location_t &backfiller,
                                                   best_backfiller_map_t *best_backfiller_out) {
    for (typename region_map_t<protocol_t, version_range_t>::const_iterator i =  offered_backfill_versions.begin();
                                                                            i != offered_backfill_versions.end();
                                                                            i++) {
        version_range_t challenger = i->second;

        best_backfiller_map_t best_backfiller_map = best_backfiller_out->mask(i->first);
        for (typename best_backfiller_map_t::iterator j =  best_backfiller_map.begin();
                                                                  j != best_backfiller_map.end();
                                                                  j++) {
            version_range_t incumbent = j->second.version_range;

            if (version_is_divergent(branch_history_manager, challenger.latest, incumbent.latest, j->first)) {
                throw divergent_data_exc_t();
            } else if (incumbent.latest == challenger.latest &&
                       incumbent.is_coherent() == challenger.is_coherent()) {
                j->second.places_to_get_this_version.push_back(backfiller);
            } else if (
                    /* `version_is_ancestor` also returns true if
                    `incumbent.latest` and `challenger.latest` are the same. So
                    if `incumbent.latest` is the ancestor of `challenger.latest`
                    *and they are distinct* then the challenger beats the
                    incumbent. */
                    (version_is_ancestor(branch_history_manager, incumbent.latest, challenger.latest, j->first) &&
                        incumbent.latest != challenger.latest) ||
                    /* If they are the same, but the challenger is coherent
                    and the incumbent is not, then that breaks the tie. */
                    (incumbent.latest == challenger.latest &&
                        challenger.is_coherent() &&
                        !incumbent.is_coherent())) {
                j->second = backfill_candidate_t(challenger,
                                                 std::vector<typename backfill_candidate_t::backfill_location_t>(1, backfiller),
                                                 false);
            } else {
                //if we get here our incumbent is better than the challenger.
                //So we don't replace it.
            }
        }
        best_backfiller_out->update(best_backfiller_map);
    }
}

template<class protocol_t>
boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > > extract_backfiller_from_reactor_business_card_secondary(
        const boost::optional<boost::optional<typename reactor_business_card_t<protocol_t>::secondary_without_primary_t> > &bcard) {
    if (!bcard) {
        return boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > >();
    } else if (!bcard.get()) {
        return boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > >(
            boost::optional<backfiller_business_card_t<protocol_t> >());
    } else {
        return boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > >(
            boost::optional<backfiller_business_card_t<protocol_t> >(
                bcard.get().get().backfiller));
    }
}

template<class protocol_t>
boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > > extract_backfiller_from_reactor_business_card_nothing(
        const boost::optional<boost::optional<typename reactor_business_card_t<protocol_t>::nothing_when_safe_t> > &bcard) {
    if (!bcard) {
        return boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > >();
    } else if (!bcard.get()) {
        return boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > >(
            boost::optional<backfiller_business_card_t<protocol_t> >());
    } else {
        return boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > >(
            boost::optional<backfiller_business_card_t<protocol_t> >(
                bcard.get().get().backfiller));
    }
}

/* Check if:
 *      - every peer is connected
 *      - has a reactor
 *      - foreach key in region peer has activity:
 *          -secondary_without_primary_t
 *          -nothing_when_safe_t
 *          -nothing_t
 *          -nothing_when_done_erasing_t
 *      - foreach key in region there's a unique latest known version available and that version is coherent
 *
 * If these conditions are met is_safe_for_us_to_be_primary will return true
 * and the best_backfiller_out parameter will contain a set of backfillers we
 * can use to get the latest version of the data.
 * Otherwise it will return false and best_backfiller_out will be unmodified.
 */
template <class protocol_t>
bool reactor_t<protocol_t>::is_safe_for_us_to_be_primary(const change_tracking_map_t<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > &_reactor_directory,
                                                         const blueprint_t<protocol_t> &blueprint,
                                                         const typename protocol_t::region_t &region, best_backfiller_map_t *best_backfiller_out,
                                                         branch_history_t<protocol_t> *branch_history_to_merge_out,
                                                         bool *merge_branch_history_out)
{
    typedef reactor_business_card_t<protocol_t> rb_t;

    best_backfiller_map_t res = *best_backfiller_out;

    /* Iterator through the peers the blueprint claims we should be able to
     * see. */
    for (typename blueprint_t<protocol_t>::role_map_t::const_iterator p_it = blueprint.peers_roles.begin();
         p_it != blueprint.peers_roles.end();
         ++p_it) {
        //The peer we are currently checking
        peer_id_t peer = p_it->first;

        if (peer == get_me()) {
            continue;
        }

        typename std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > >::const_iterator bcard_it = _reactor_directory.get_inner().find(p_it->first);
        if (bcard_it == _reactor_directory.get_inner().end()) {
            return false;
        }

        std::vector<typename protocol_t::region_t> regions;
        for (typename rb_t::activity_map_t::const_iterator it = bcard_it->second->activities.begin();
             it != bcard_it->second->activities.end();
             ++it) {
            typename protocol_t::region_t intersection = region_intersection(it->second.region, region);
            if (!region_is_empty(intersection)) {
                regions.push_back(intersection);
                try {
                    if (const typename rb_t::secondary_without_primary_t * secondary_without_primary = boost::get<typename rb_t::secondary_without_primary_t>(&it->second.activity)) {
                        branch_history_t<protocol_t> bh = secondary_without_primary->branch_history;
                        std::set<branch_id_t> known_branchs = branch_history_manager->known_branches();

                        for (typename std::map<branch_id_t, branch_birth_certificate_t<protocol_t> >::iterator bt  = bh.branches.begin();
                                bt != bh.branches.end(); ++bt) {
                            if (!std_contains(known_branchs, bt->first)) {
                                *branch_history_to_merge_out = bh;
                                *merge_branch_history_out = true;
                                return true;
                            }
                        }

                        update_best_backfiller(secondary_without_primary->current_state,
                                               typename backfill_candidate_t::backfill_location_t(
                                                   get_directory_entry_view<typename rb_t::secondary_without_primary_t>(peer, it->first)->subview(
                                                        &extract_backfiller_from_reactor_business_card_secondary<protocol_t>),
                                                   peer,
                                                   it->first),
                                               &res);
                    } else if (const typename rb_t::nothing_when_safe_t * nothing_when_safe = boost::get<typename rb_t::nothing_when_safe_t>(&it->second.activity)) {
                        branch_history_t<protocol_t> bh = nothing_when_safe->branch_history;
                        std::set<branch_id_t> known_branchs = branch_history_manager->known_branches();

                        for (typename std::map<branch_id_t, branch_birth_certificate_t<protocol_t> >::iterator bt  = bh.branches.begin();
                                bt != bh.branches.end(); ++bt) {
                            if (!std_contains(known_branchs, bt->first)) {
                                *branch_history_to_merge_out = bh;
                                *merge_branch_history_out = true;
                                return true;
                            }
                        }

                        update_best_backfiller(nothing_when_safe->current_state,
                                               typename backfill_candidate_t::backfill_location_t(
                                                   get_directory_entry_view<typename rb_t::nothing_when_safe_t>(peer, it->first)->subview(
                                                        &extract_backfiller_from_reactor_business_card_nothing<protocol_t>),
                                                   peer,
                                                   it->first),
                                               &res);
                    } else if (boost::get<typename rb_t::nothing_t>(&it->second.activity)) {
                        //Everything's fine this peer cannot obstruct us so we shall proceed
                    } else if (boost::get<typename rb_t::nothing_when_done_erasing_t>(&it->second.activity)) {
                        //Everything's fine this peer cannot obstruct us so we shall proceed
                    } else {
                        return false;
                    }
                } catch (const divergent_data_exc_t &) {
                    return false;
                }
            }
        }
        /* This returns false if the peers reactor is missing an activity for
         * some sub region. It crashes if they overlap. */
        typename protocol_t::region_t join;
        region_join_result_t join_result = region_join(regions, &join);

        switch (join_result) {
        case REGION_JOIN_OK:
            if (join != region) {
                return false;
            }
            break;
        case REGION_JOIN_BAD_REGION:
            return false;
        case REGION_JOIN_BAD_JOIN:
            crash("Overlapping activities in directory, this can only happen due to programmer error.\n");
        default:
            unreachable();
        }
    }

    /* If the latest version of the data we've found for a region is incoherent
     * then we don't backfill it automatically. The admin must explicit "bless"
     * the incoherent data making it coherent or get rid of all incoherent data
     * until the most up to date data for a region is coherent. */
    for (typename best_backfiller_map_t::iterator it = res.begin(); it != res.end(); ++it) {
        if (!it->second.version_range.is_coherent()) {
            return false;
        }
    }

    *best_backfiller_out = res;

    *merge_branch_history_out = false;
    return true;
}

/* This function helps initialize best backfiller maps, by constructing
 * candidates that represent the current state of the data in our store. This
 * candidates tell us not to backfill that data if our local store's version is
 * alreadly the most up to date. */
template <class protocol_t>
typename reactor_t<protocol_t>::backfill_candidate_t reactor_t<protocol_t>::make_backfill_candidate_from_version_range(const version_range_t &vr) {
    return backfill_candidate_t(vr, std::vector<typename backfill_candidate_t::backfill_location_t>(), true);
}

/* Wraps backfillee, catches the exceptions it throws and instead uses a
 * promise to indicate success or failure (not throwing or throwing) */
template <class protocol_t>
void do_backfill(
        mailbox_manager_t *mailbox_manager,
        branch_history_manager_t<protocol_t> *branch_history_manager,
        store_view_t<protocol_t> *svs,
        typename protocol_t::region_t region,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > > > > backfiller_metadata,
        backfill_session_id_t backfill_session_id,
        promise_t<bool> *success,
        signal_t *interruptor) THROWS_NOTHING {

    bool result = false;

    try {
        cross_thread_watchable_variable_t<boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > > > ct_backfiller_metadata(backfiller_metadata, svs->home_thread());
        cross_thread_signal_t ct_interruptor(interruptor, svs->home_thread());
        on_thread_t th(svs->home_thread());

        backfillee<protocol_t>(mailbox_manager, branch_history_manager, svs, region, ct_backfiller_metadata.get_watchable(), backfill_session_id, &ct_interruptor);

        result = true;
    } catch (const interrupted_exc_t &) {
    } catch (const resource_lost_exc_t &) {
    }

    success->pulse(result);
}

template <class protocol_t>
bool check_that_we_see_our_broadcaster(const boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > &maybe_a_business_card) {
    guarantee(maybe_a_business_card, "Not connected to ourselves\n");
    return maybe_a_business_card.get();
}

template <class protocol_t>
bool reactor_t<protocol_t>::attempt_backfill_from_peers(directory_entry_t *directory_entry,
                                                        order_source_t *order_source,
                                                        const typename protocol_t::region_t &region,
                                                        store_view_t<protocol_t> *svs,
                                                        const clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > &blueprint,
                                                        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    cross_thread_signal_t ct_interruptor(interruptor, svs->home_thread());
    on_thread_t th(svs->home_thread());

    /* Figure out what version of the data is already present in our
     * store so we don't backfill anything prior to it. */
    object_buffer_t<fifo_enforcer_sink_t::exit_read_t> read_token;
    svs->new_read_token(&read_token);
    region_map_t<protocol_t, binary_blob_t> metainfo_blob;
    svs->do_get_metainfo(order_source->check_in("reactor_t::be_primary").with_read_mode(), &read_token, &ct_interruptor, &metainfo_blob);

    on_thread_t th2(this->home_thread());

    region_map_t<protocol_t, version_range_t> metainfo = to_version_range_map(metainfo_blob);
    region_map_t<protocol_t, backfill_candidate_t> best_backfillers = region_map_transform<protocol_t, version_range_t, backfill_candidate_t>(metainfo, &reactor_t<protocol_t>::make_backfill_candidate_from_version_range);

    /* This waits until every other peer is ready to accept us as the
     * primary and there is a unique coherent latest verstion of the
     * data available. Note best_backfillers is passed as an
     * input/output parameter, after this call returns best_backfillers
     * will describe how to fill the store with the most up-to-date
     * data. */

    /* Notice this run until satisfied can return for one of two reasons.
     * Either it returns because it is safe for us to be primary or it returns
     * because we found someone with a branch history we didn't know about and
     * we need to merge it in. This has to be done outside of run until
     * satisfied because it can't block. */

    while (true) {
        branch_history_t<protocol_t> branch_history_to_merge;
        bool i_should_merge_branch_history = false;
        run_until_satisfied_2(directory_echo_mirror.get_internal(),
                              blueprint,
                              boost::bind(&reactor_t<protocol_t>::is_safe_for_us_to_be_primary, this, _1, _2, region, &best_backfillers, &branch_history_to_merge, &i_should_merge_branch_history),
                              interruptor,
                              REACTOR_RUN_UNTIL_SATISFIED_NAP);
        if (i_should_merge_branch_history) {
            branch_history_manager->import_branch_history(branch_history_to_merge, interruptor);
        } else {
            break;
        }
    }

    /* We may be backfilling from several sources, each requires a
     * promise be passed in which gets pulsed with a value indicating
     * whether or not the backfill succeeded. */
    boost::ptr_vector<promise_t<bool> > promises;

    std::vector<reactor_business_card_details::backfill_location_t> backfills;

    for (typename best_backfiller_map_t::iterator it =  best_backfillers.begin();
         it != best_backfillers.end();
         ++it) {
        if (it->second.present_in_our_store) {
            continue;
        } else {
            backfill_session_id_t backfill_session_id = generate_uuid();
            promise_t<bool> *p = new promise_t<bool>;
            promises.push_back(p);
            coro_t::spawn_sometime(boost::bind(&do_backfill<protocol_t>,
                                               mailbox_manager,
                                               branch_history_manager,
                                               svs,
                                               it->first,
                                               it->second.places_to_get_this_version[0].backfiller,
                                               backfill_session_id,
                                               p,
                                               interruptor));
            reactor_business_card_details::backfill_location_t backfill_location(backfill_session_id,
                                                                                 it->second.places_to_get_this_version[0].peer_id,
                                                                                 it->second.places_to_get_this_version[0].activity_id);

            backfills.push_back(backfill_location);
        }
    }

    /* Tell the other peers which backfills we're waiting on. */
    directory_entry->set(typename reactor_business_card_t<protocol_t>::primary_when_safe_t(backfills));

    /* Since these don't actually modify peers behavior, just allow
     * them to query the backfiller for progress reports there's no
     * need to wait for acks. */

    bool all_succeeded = true;
    for (typename boost::ptr_vector<promise_t<bool> >::iterator it = promises.begin();
         it != promises.end();
         ++it) {
        //DO NOT switch the order of it->wait() and all_succeeded
        all_succeeded = it->wait() && all_succeeded;
    }

    /* If the interruptor was pulsed the interrupted_exc_t would have
     * been caugh in the do_backfill coro, we need to check if that's
     * how we got here and rethrow the exception. */
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    // Return whether we have the most up to date version of the data in our store.
    return all_succeeded;
}

template<class protocol_t>
void reactor_t<protocol_t>::be_primary(typename protocol_t::region_t region, store_view_t<protocol_t> *svs, const clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > &blueprint, signal_t *interruptor) THROWS_NOTHING {
    try {
        //Tell everyone that we're looking to become the primary
        directory_entry_t directory_entry(this, region);

        order_source_t order_source(svs->home_thread());  // TODO: order_token_t::ignore

        /* Tell everyone watching our directory entry what we're up to. */
        directory_echo_version_t version_to_wait_on = directory_entry.set(typename reactor_business_card_t<protocol_t>::primary_when_safe_t());

        /* block until all peers have acked `directory_entry` */
        wait_for_directory_acks(version_to_wait_on, interruptor);


        /* In this loop we repeatedly attempt to find peers to backfill from
         * and then perform the backfill. We exit the loop either when we get
         * interrupted or we have backfilled the most up to date data. */
        while (!attempt_backfill_from_peers(&directory_entry, &order_source, region, svs, blueprint, interruptor)) { }

        // TODO: Don't use local stack variable.
        std::string region_name = strprintf("be_primary_%p", &region);

        cross_thread_signal_t ct_interruptor(interruptor, svs->home_thread());
        on_thread_t th(svs->home_thread());

        perfmon_collection_t region_perfmon_collection;
        perfmon_membership_t region_perfmon_membership(&regions_perfmon_collection, &region_perfmon_collection, region_name);

        broadcaster_t<protocol_t> broadcaster(mailbox_manager, branch_history_manager, svs, &region_perfmon_collection, &order_source, &ct_interruptor);

        on_thread_t th2(this->home_thread());

        directory_entry.set(typename reactor_business_card_t<protocol_t>::primary_t(broadcaster.get_business_card()));

        clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster_business_card =
            get_directory_entry_view<typename reactor_business_card_t<protocol_t>::primary_t>(get_me(), directory_entry.get_reactor_activity_id())->
                subview(&reactor_t<protocol_t>::extract_broadcaster_from_reactor_business_card_primary);

        /* listener_t expects broadcaster to be visible in the directory at the
         * time that it's constructed. It might take some time to propogate to
         * ourselves after we've put it in the directory. */
        broadcaster_business_card->run_until_satisfied(
            &check_that_we_see_our_broadcaster<protocol_t>, interruptor,
            REACTOR_RUN_UNTIL_SATISFIED_NAP);

        cross_thread_watchable_variable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > ct_broadcaster_business_card(broadcaster_business_card, svs->home_thread());

        on_thread_t th3(svs->home_thread());
        listener_t<protocol_t> listener(base_path, io_backender, mailbox_manager, ct_broadcaster_business_card.get_watchable(), branch_history_manager, &broadcaster, &region_perfmon_collection, &ct_interruptor, &order_source);
        replier_t<protocol_t> replier(&listener, mailbox_manager, branch_history_manager);
        master_t<protocol_t> master(mailbox_manager, ack_checker, region, &broadcaster);
        direct_reader_t<protocol_t> direct_reader(mailbox_manager, svs);

        on_thread_t th4(this->home_thread());

        directory_entry.update_without_changing_id(
            typename reactor_business_card_t<protocol_t>::primary_t(
                broadcaster.get_business_card(),
                replier.get_business_card(),
                master.get_business_card(),
                direct_reader.get_business_card()));

        interruptor->wait_lazily_unordered();

    } catch (const interrupted_exc_t &) {
        /* ignore */
    }
}


#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
#include "memcached/protocol.hpp"
#include "memcached/protocol_json_adapter.hpp"
#include "rdb_protocol/protocol.hpp"


template void reactor_t<mock::dummy_protocol_t>::be_primary(mock::dummy_protocol_t::region_t region, store_view_t<mock::dummy_protocol_t> *svs, const clone_ptr_t<watchable_t<blueprint_t<mock::dummy_protocol_t> > > &blueprint, signal_t *interruptor) THROWS_NOTHING;
template void reactor_t<memcached_protocol_t>::be_primary(memcached_protocol_t::region_t region, store_view_t<memcached_protocol_t> *svs, const clone_ptr_t<watchable_t<blueprint_t<memcached_protocol_t> > > &blueprint, signal_t *interruptor) THROWS_NOTHING;
template void reactor_t<rdb_protocol_t>::be_primary(rdb_protocol_t::region_t region, store_view_t<rdb_protocol_t> *svs, const clone_ptr_t<watchable_t<blueprint_t<rdb_protocol_t> > > &blueprint, signal_t *interruptor) THROWS_NOTHING;
