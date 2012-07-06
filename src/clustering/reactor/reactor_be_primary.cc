#include "clustering/reactor/reactor.hpp"

#include <exception>
#include <vector>

#include "clustering/administration/http/json_adapters.hpp"
#include "clustering/immediate_consistency/branch/backfillee.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/direct_reader.hpp"

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
            } else if (version_is_ancestor(branch_history_manager, incumbent.latest, challenger.latest, j->first) ||
                       (incumbent.latest == challenger.latest && challenger.is_coherent() && !incumbent.is_coherent())) {
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
bool reactor_t<protocol_t>::is_safe_for_us_to_be_primary(const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > &reactor_directory, const blueprint_t<protocol_t> &blueprint,
                                                         const typename protocol_t::region_t &region, best_backfiller_map_t *best_backfiller_out)
{
    typedef reactor_business_card_t<protocol_t> rb_t;

    best_backfiller_map_t res = *best_backfiller_out;

    /* Iterator through the peers the blueprint claims we should be able to
     * see. */
    for (typename blueprint_t<protocol_t>::role_map_t::const_iterator p_it =  blueprint.peers_roles.begin();
                                                                      p_it != blueprint.peers_roles.end();
                                                                      p_it++) {
        //The peer we are currently checking
        peer_id_t peer = p_it->first;

        if (peer == get_me()) {
            continue;
        }

        typename std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > >::const_iterator bcard_it = reactor_directory.find(p_it->first);
        if (bcard_it == reactor_directory.end() ||
            !bcard_it->second) {
            return false;
        }

        std::vector<typename protocol_t::region_t> regions;
        for (typename rb_t::activity_map_t::const_iterator it =  (*bcard_it->second).internal.activities.begin();
                                                           it != (*bcard_it->second).internal.activities.end();
                                                           it++) {
            typename protocol_t::region_t intersection = region_intersection(it->second.first, region);
            if (!region_is_empty(intersection)) {
                regions.push_back(intersection);
                try {
                    if (const typename rb_t::secondary_without_primary_t * secondary_without_primary = boost::get<typename rb_t::secondary_without_primary_t>(&it->second.second)) {
                        update_best_backfiller(secondary_without_primary->current_state,
                                               typename backfill_candidate_t::backfill_location_t(
                                                   get_directory_entry_view<typename rb_t::secondary_without_primary_t>(peer, it->first)->subview(
                                                        &extract_backfiller_from_reactor_business_card_secondary<protocol_t>),
                                                   peer,
                                                   it->first),
                                               &res);
                    } else if (const typename rb_t::nothing_when_safe_t * nothing_when_safe = boost::get<typename rb_t::nothing_when_safe_t>(&it->second.second)) {
                        update_best_backfiller(nothing_when_safe->current_state,
                                               typename backfill_candidate_t::backfill_location_t(
                                                   get_directory_entry_view<typename rb_t::nothing_when_safe_t>(peer, it->first)->subview(
                                                        &extract_backfiller_from_reactor_business_card_nothing<protocol_t>),
                                                   peer,
                                                   it->first),
                                               &res);
                    } else if(boost::get<typename rb_t::nothing_t>(&it->second.second)) {
                        //Everything's fine this peer cannot obstruct us so we shall proceed
                    } else if(boost::get<typename rb_t::nothing_when_done_erasing_t>(&it->second.second)) {
                        //Everything's fine this peer cannot obstruct us so we shall proceed
                    } else {
                        return false;
                    }
                } catch (divergent_data_exc_t) {
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
     * until the must up to date data for a region is coherent. */
    for (typename best_backfiller_map_t::iterator it =  res.begin();
                                                  it != res.end();
                                                  it++) {
        if (!it->second.version_range.is_coherent()) {
            return false;
        }
    }

    *best_backfiller_out = res;
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
        multistore_ptr_t<protocol_t> *svs,
        typename protocol_t::region_t region,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > > > > backfiller_metadata,
        backfill_session_id_t backfill_session_id,
        promise_t<bool> *success,
        signal_t *interruptor) THROWS_NOTHING {
    try {
        backfillee<protocol_t>(mailbox_manager, branch_history_manager, svs, region, backfiller_metadata, backfill_session_id, interruptor);
        success->pulse(true);
    } catch (interrupted_exc_t) {
        success->pulse(false);
    } catch (resource_lost_exc_t) {
        success->pulse(false);
    }

}

template <class protocol_t>
bool check_that_we_see_our_broadcaster(const boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > &maybe_a_business_card) {
    rassert(maybe_a_business_card, "Not connected to ourselves\n");
    return bool(maybe_a_business_card.get());
}

template<class protocol_t>
void reactor_t<protocol_t>::be_primary(typename protocol_t::region_t region, multistore_ptr_t<protocol_t> *svs, const clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > &blueprint, signal_t *interruptor) THROWS_NOTHING {
    try {
        //Tell everyone that we're looking to become the primary
        directory_entry_t directory_entry(this, region);

        /* In this loop we repeatedly attempt to find peers to backfill from
         * and then perform the backfill. We exit the loop either when we get
         * interrupted or we have backfilled the most up to date data. */
        while (true) {
            directory_echo_version_t version_to_wait_on = directory_entry.set(typename reactor_business_card_t<protocol_t>::primary_when_safe_t());

            /* block until all peers have acked `directory_entry` */
            wait_for_directory_acks(version_to_wait_on, interruptor);

            /* Figure out what version of the data is already present in our
             * store so we don't backfill anything prior to it. */
            scoped_array_t<boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> > read_tokens;
            svs->new_read_tokens(&read_tokens);
            region_map_t<protocol_t, version_range_t> metainfo = svs->get_all_metainfos(order_token_t::ignore, read_tokens, interruptor);
            region_map_t<protocol_t, backfill_candidate_t> best_backfillers = region_map_transform<protocol_t, version_range_t, backfill_candidate_t>(metainfo, &reactor_t<protocol_t>::make_backfill_candidate_from_version_range);

            /* This waits until every other peer is ready to accept us as the
             * primary and there is a unique coherent latest verstion of the
             * data available. Note best_backfillers is passed as an
             * input/output parameter, after this call returns best_backfillers
             * will describe how to fill the store with the most up-to-date
             * data. */
            run_until_satisfied_2(
                reactor_directory,
                blueprint,
                boost::bind(&reactor_t<protocol_t>::is_safe_for_us_to_be_primary, this, _1, _2, region, &best_backfillers),
                interruptor);

            /* We may be backfilling from several sources, each requires a
             * promise be passed in which gets pulsed with a value indicating
             * whether or not the backfill succeeded. */
            boost::ptr_vector<promise_t<bool> > promises;

            std::vector<reactor_business_card_details::backfill_location_t> backfills;

            for (typename best_backfiller_map_t::iterator it =  best_backfillers.begin();
                                                          it != best_backfillers.end();
                                                          it++) {
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
            directory_entry.set(typename reactor_business_card_t<protocol_t>::primary_when_safe_t(backfills));

            /* Since these don't actually modify peers behavior, just allow
             * them to query the backfiller for progress reports there's no
             * need to wait for acks. */

            bool all_succeeded = true;
            for (typename boost::ptr_vector<promise_t<bool> >::iterator it =  promises.begin();
                                                                        it != promises.end();
                                                                        it++) {
                //DO NOT switch the order of it->wait() and all_succeeded
                all_succeeded = it->wait() && all_succeeded;
            }

            /* If the interruptor was pulsed the interrupted_exc_t would have
             * been caugh in the do_backfill coro, we need to check if that's
             * how we got here and rethrow the exception. */
            if (interruptor->is_pulsed()) {
                throw interrupted_exc_t();
            }

            if (all_succeeded) {
                /* We have the most up to date version of the data in our
                 * store. */
                break;
            } else {
                /* one or more backfills failed so we start the whole process
                 * over again..... */
            }
        }

        std::string region_name(render_region_as_string(&region, 0));
        perfmon_collection_t region_perfmon_collection;
        perfmon_membership_t region_perfmon_membership(&regions_perfmon_collection, &region_perfmon_collection, region_name);

        broadcaster_t<protocol_t> broadcaster(mailbox_manager, branch_history_manager, svs, &region_perfmon_collection, interruptor);

        directory_entry.set(typename reactor_business_card_t<protocol_t>::primary_t(broadcaster.get_business_card()));

        clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster_business_card =
            get_directory_entry_view<typename reactor_business_card_t<protocol_t>::primary_t>(get_me(), directory_entry.get_reactor_activity_id())->
                subview(&reactor_t<protocol_t>::extract_broadcaster_from_reactor_business_card_primary);

        /* listener_t expects broadcaster to be visible in the directory at the
         * time that it's constructed. It might take some time to propogate to
         * ourselves after we've put it in the directory. */
        broadcaster_business_card->run_until_satisfied(&check_that_we_see_our_broadcaster<protocol_t>, interruptor);

        listener_t<protocol_t> listener(io_backender, mailbox_manager, broadcaster_business_card, branch_history_manager, &broadcaster, &region_perfmon_collection, interruptor);
        replier_t<protocol_t> replier(&listener);
        master_t<protocol_t> master(mailbox_manager, ack_checker, region, &broadcaster);
        direct_reader_t<protocol_t> direct_reader(mailbox_manager, svs);

        directory_entry.update_without_changing_id(
            typename reactor_business_card_t<protocol_t>::primary_t(
                broadcaster.get_business_card(),
                replier.get_business_card(),
                master.get_business_card(),
                direct_reader.get_business_card()
            ));

        interruptor->wait_lazily_unordered();
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}


#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
#include "memcached/protocol.hpp"
#include "memcached/protocol_json_adapter.hpp"

template class reactor_t<mock::dummy_protocol_t>;
template class reactor_t<memcached_protocol_t>;

