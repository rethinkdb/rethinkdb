#ifndef __CLUSTERING_REACTOR_REACTOR_BE_PRIMARY_TCC__
#define __CLUSTERING_REACTOR_REACTOR_BE_PRIMARY_TCC__

template <class protocol_t>
class best_backfiller_map_t {
    region_map_t<protocol_t, std::pair<clone_ptr_t<directory_single_rview_t<boost::optional<backfiller_business_card_t<protocol_t> > > >, version_t> val;
};

template <class protocol_t>
class peer_activity_is_safe_for_us_to_be_primary_and_perhaps_add_backfiller_map_t : public boost::static_visitor<bool> {
public:
    //bad states
    bool operator()(const typename reactor_business_card_t<protocol_t>::primary_when_safe_t &) const {
        return false;
    }

    bool operator()(const typename reactor_business_card_t<protocol_t>::primary_t &) const {
        return false;
    }

    bool operator()(const typename reactor_business_card_t<protocol_t>::secondary_up_to_date_t &) const {
        return false;
    }

    bool operator()(const typename reactor_business_card_t<protocol_t>::secondary_backfilling_t &) const {
        return false;
    }

    bool operator()(const typename reactor_business_card_t<protocol_t>::listener_backfilling_t &) const {
        return false;
    }

    bool operator()(const typename reactor_business_card_t<protocol_t>::listener_up_to_date_t &) const {
        return false;
    }


    //good
    bool operator()(const typename reactor_business_card_t<protocol_t>::secondary_without_primary_t &) const {
        return true;
    }

    bool operator()(const typename reactor_business_card_t<protocol_t>::listener_without_primary_t &) const {
        return true;
    }

    bool operator()(const typename reactor_business_card_t<protocol_t>::nothing_when_safe_t &) const {
        return true;
    }

    bool operator()(const typename reactor_business_card_t<protocol_t>::nothing_t &) const {
        return true;
    }

    bool operator()(const typename reactor_business_card_t<protocol_t>::nothing_when_done_erasing_t &) const {
        return true;
    }
};

/* Check if peer is connected, has a reactor and foreach key in region: peer
 * has activity secondary_without_primary_t, listener_without_primary_t,
 * nothing_when_safe_t, nothing_t or nothing_when_done_erasing_t.
 */
template <class protocol_t>
bool is_safe_for_us_to_be_primary(const boost::optional<boost::optional<reactor_business_card_t<protocol_t> > > &bcard, const typename protocol_t::region_t &region, best_backfiller_map_t<protocol_t> *best_backfiller_out) {
    best_backfiller_map_t<protocol_t> res = *best_backfiller_out;

    if (!bcard || !*bcard) {
        return false;
    }

    std::vector<typename protocol_t::region_t> regions;
    for (typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator it =  (**bcard).activities.begin();
                                                                                      it != (**bcard).activities.end();
                                                                                      it++) {
        protocol_t::region_t intersection = region_intersection(it->second.first, region);
        if (!region_is_empty(intersection)) {
            regions.push_back(intersection);
            if (!boost::apply_visitor(peer_activity_is_safe_for_us_to_be_primary_t<protocol_t>(), it->second.second)) {
                return false;
            }

        }
    }
    try {
        if (region_join(regions) != region) {
            return false;
        }
    catch (bad_region_exc_t) {
        return false;
    } catch (bad_join_exc_t) {
        crash("Overlapping activities in directory, this can only happen due to programmer error.\n");
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::be_primary(typename protocol_t::region_t region, store_view_t<protocol_t> *store, const blueprint_t<protocol_t> &blueprint, signal_t *interruptor) THROWS_NOTHING {
    try {
        directory_entry_t directory_entry(this, region);
        directory_echo_version_t version_to_wait_on = directory_entry.set(typename reactor_business_card_t<protocol_t>::primary_when_safe_t());

        /* block until all peers have acked `directory_entry` */
        wait_for_directory_acks(version_to_wait_on, interruptor);

        for (std::set<peer_id_t>::iterator it =  blueprint.activities.begin();
                                           it != blueprint.activities.end();
                                           it++) {
            boost::optional<reactor_business_card_t<protocol_t> > bcard = directory_echo_access.get_internal_view()->get_peer_view(*it)->get_value_which_satisfies_predicate(boost::bind(&is_safe_for_us_to_be_primary, _1, region)).get();
        }


        /* TODO foreach key in region: backfill from most up-to-date
           SECONDARY_LOST or NOTHING_SOON */
        broadcaster_t<protocol_t> broadcaster(mailbox_manager, branch_history, store, interruptor);

        directory_entry.set(typename reactor_business_card_t<protocol_t>::primary_t(broadcaster.get_business_card()));

        clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<protocol_t> > > > broadcaster_business_card =
            get_directory_entry_view<typename reactor_business_card_t<protocol_t>::primary_t>(get_me(), directory_entry.get_reactor_activity_id())->
                subview(optional_monad_lens<broadcaster_business_card_t<protocol_t>, typename reactor_business_card_t<protocol_t>::primary_t>(
                    field_lens(&reactor_business_card_t<protocol_t>::primary_t::broadcaster)));

        listener_t<protocol_t> listener(mailbox_manager, broadcaster_business_card, branch_history, &broadcaster, interruptor);
        replier_t<protocol_t> replier(&listener);
        master_t<protocol_t> master(mailbox_manager, master_directory, region, &broadcaster);

        directory_entry.update_without_changing_id(typename reactor_business_card_t<protocol_t>::primary_t(broadcaster.get_business_card(), replier.get_business_card()));

        interruptor->wait_lazily_unordered();
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

#endif
