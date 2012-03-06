#ifndef __CLUSTERING_SUGGESTER_SUGGESTER_TCC__
#define __CLUSTERING_SUGGESTER_SUGGESTER_TCC__

/* Returns a "score" indicating how expensive it would be to turn the machine
with the given business card into a primary or secondary for the given shard. */

template<class protocol_t>
float estimate_cost_to_get_up_to_date(
        const reactor_business_card_t<protocol_t> &business_card,
        const typename protocol_t::region_t &shard) {
    typedef reactor_business_card_t<protocol_t> rb_t;
    region_map_t<protocol_t, float> costs(shard, 3);
    for (typename rb_t::activity_map_t::const_iterator it = business_card.activity_map.begin();
            it != business_card.activity_map.end(); it++) {
        int cost;
        if (boost::get<typename rb_t::primary_when_safe_t>(&it->second.second)) {
            cost = 0;
        } else if (boost::get<typename rb_t::primary_t>(&it->second.second)) {
            cost = 0;
        } else if (boost::get<typename rb_t::secondary_up_to_date_t>(&it->second.second)) {
            cost = 1;
        } else if (boost::get<typename rb_t::secondary_without_primary_t>(&it->second.second)) {
            cost = 2;
        } else if (boost::get<typename rb_t::secondary_backfilling_t>(&it->second.second)) {
            cost = 2;
        } else if (boost::get<typename rb_t::nothing_when_safe_t>(&it->second.second)) {
            cost = 3;
        } else if (boost::get<typename rb_t::nothing_when_done_erasing_t>(&it->second.second)) {
            cost = 3;
        } else if (boost::get<typename rb_t::nothing_t>(&it->second.second)) {
            cost = 3;
        }
        /* It's ok to just call `set()` instead of trying to find the minimum
        because activities should never overlap. */
        costs.set(it->second.first, cost);
    }
    float sum = 0;
    int count = 0;
    for (typename region_map_t<protocol_t, float>::iterator it = costs.begin(); it != costs.end(); it++) {
        /* TODO: Scale by how much data is in `it->first` */
        sum += it->second;
        count++;
    }
    return sum / count;
}

inline std::vector<peer_id_t> pick_n_best(const std::multimap<float, peer_id_t> candidates, int n) {
    std::vector<peer_id_t> result;
    rng_t rng;
    std::multimap<float, peer_id_t>::iterator it = candidates.begin();
    while (result.size() < n) {
        if (it == candidates.end()) {
            throw cannot_satisfy_goals_exc_t();
        }
        float block_value = it->first;
        std::vector<peer_id_t> block;
        while (it != candidates.upper_bound(block_value)) {
            block.push_back(it->second);
            it++;
        }
        random_shuffle(block.begin(), block.end(), boost::bind(&rng_t::randint, &rng, _1));
        int i = 0;
        while (result.size() < n && i < block.size()) {
            result.push_back(block[i]);
            i++;
        }
    }
    return result;
}

template<class protocol_t>
std::map<peer_id_t, typename blueprint_t<protocol_t>::role_t> suggest_blueprint_for_shard(
        const std::map<peer_id_t, reactor_business_card_t<protocol_t> > &directory,
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &datacenter_affinities,
        const typename protocol_t::region_t &shard,
        const std::map<peer_id_t, datacenter_id_t> &machine_data_centers) {

    std::map<peer_id_t, typename blueprint_t<protocol_t>::role_t> blueprint;

    std::multimap<float, peer_id_t> primary_candidates;
    for (std::map<peer_id_t, datacenter_id_t>::const_iterator it = machine_data_centers.begin();
            it != machine_data_centers.end(); it++) {
        if (it->second == primary_datacenter) {
            primary_candidates.insert(estimate_cost_to_get_up_to_date(directory[it->first], shard));
        }
    }
    peer_id_t primary = pick_n_best(primary_candidates, 1).front();
    blueprint[primary] = blueprint_t<protocol_t>::role_primary;

    for (std::map<datacenter_id_t, int>::const_iterator it = datacenter_affinities.begin(); it != datacenter_affinities.end(); it++) {
        std::multimap<float, peer_id_t> secondary_candidates;
        for (std::map<peer_id_t, datacenter_id_t>::const_iterator jt = machine_data_centers.begin();
                jt != machine_data_centers.end(); jt++) {
            if (jt->second == it->first && jt->first != primary) {
                primary_candidates.insert(estimate_cost_to_get_up_to_date(directory[jt->first], shard));
            }
        }
        std::vector<peer_id_t> secondaries = pick_n_best(secondary_candidates, it->second);
        for (std::vector<peer_id_t>::iterator jt = secondaries.begin(); jt != secondaries.end(); jt++) {
            blueprint[*jt] = blueprint_t<protocol_t>::role_secondary;
        }
    }

    for (std::map<peer_id_t, datacenter_id_t>::const_iterator it = machine_data_centers.begin();
            it != machine_data_centers.end(); it++) {
        std::pair<typename std::map<peer_id_t, typename blueprint_t<protocol_t>::role_t>::iterator, bool> insertion_result =
            blueprint.insert(it->first);
        if (insertion_result.second) {
            /* This machine was not chosen as a primary or secondary. So mark it
            as a nothing. */
            insertion_result.first->second = blueprint_t<protocol_t>::role_nothing;
        }
    }

    return blueprint;
}

template<class protocol_t>
blueprint_t<protocol_t> suggest_blueprint(
        const std::map<peer_id_t, reactor_business_card_t<protocol_t> > &directory,
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &datacenter_affinities,
        const std::set<typename protocol_t::region_t> &shards,
        const std::map<peer_id_t, datacenter_id_t> &machine_data_centers) {

    blueprint_t<protocol_t> blueprint;
    for (typename std::set<typename protocol_t::region_t>::const_iterator it = shards.begin();
            it != shards.end(); it++) {
        std::map<peer_id_t, typename blueprint_t<protocol_t>::role_t> shard_blueprint =
            suggest_blueprint_for_shard(directory, primary_datacenter, datacenter_affinities, *it, machine_data_centers);
        for (typename std::map<peer_id_t, typename blueprint_t<protocol_t>::role_t>::iterator jt = shard_blueprint.begin();
                jt != shard_blueprint.end(); jt++) {
            blueprint[jt->first][*it] = jt->second;
        }
    }
    return blueprint;
}

#endif /* __CLUSTERING_SUGGESTER_SUGGESTER_TCC__ */
