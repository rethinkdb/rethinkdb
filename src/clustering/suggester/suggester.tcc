#ifndef CLUSTERING_SUGGESTER_SUGGESTER_TCC_
#define CLUSTERING_SUGGESTER_SUGGESTER_TCC_

#include "stl_utils.hpp"

/* Returns a "score" indicating how expensive it would be to turn the machine
with the given business card into a primary or secondary for the given shard. */

template<class protocol_t>
float estimate_cost_to_get_up_to_date(
        const reactor_business_card_t<protocol_t> &business_card,
        const typename protocol_t::region_t &shard) {
    typedef reactor_business_card_t<protocol_t> rb_t;
    region_map_t<protocol_t, float> costs(shard, 3);
    for (typename rb_t::activity_map_t::const_iterator it = business_card.activities.begin();
            it != business_card.activities.end(); it++) {
        typename protocol_t::region_t intersection = region_intersection(it->second.first, shard);
        if (!region_is_empty(intersection)) {
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
            } else {
                // I don't know if this is unreachable, but cost would be uninitialized otherwise  - Sam
                // TODO: Is this really unreachable?
                unreachable();
            }
            /* It's ok to just call `set()` instead of trying to find the minimum
            because activities should never overlap. */
            costs.set(intersection, cost);
        }
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

inline std::vector<machine_id_t> pick_n_best(const std::multimap<float, machine_id_t> candidates, int n, const datacenter_id_t &datacenter) {
    std::vector<machine_id_t> result;
    std::multimap<float, machine_id_t>::const_iterator it = candidates.begin();
    while ((int)result.size() < n) {
        if (it == candidates.end()) {
            throw cannot_satisfy_goals_exc_t(strprintf("Didn't have enough unused machines in datacenter %s, we needed %d\n", uuid_to_str(datacenter).c_str(), n));
        }
        float block_value = it->first;
        std::vector<machine_id_t> block;
        while (it != candidates.upper_bound(block_value)) {
            block.push_back(it->second);
            it++;
        }
        std::random_shuffle(block.begin(), block.end());
        int i = 0;
        while ((int)result.size() < n && i < (int)block.size()) {
            result.push_back(block[i]);
            i++;
        }
    }
    return result;
}

template<class protocol_t>
std::map<machine_id_t, typename blueprint_details::role_t> suggest_blueprint_for_shard(
        const std::map<machine_id_t, reactor_business_card_t<protocol_t> > &directory,
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &datacenter_affinities,
        const typename protocol_t::region_t &shard,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const std::set<machine_id_t> &pinnings) {

    std::map<machine_id_t, typename blueprint_details::role_t> sub_blueprint;

    std::multimap<float, machine_id_t> primary_candidates;
    for (std::map<machine_id_t, datacenter_id_t>::const_iterator it = machine_data_centers.begin();
            it != machine_data_centers.end(); it++) {
        if (it->second == primary_datacenter) {
            float cost;
            if (std_contains(directory, it->first)) {
                cost = estimate_cost_to_get_up_to_date(directory.find(it->first)->second, shard);
            } else {
                cost = 3.0;
            }

            if (!std_contains(pinnings, it->first)) {
                cost += 4.0;
            }

            primary_candidates.insert(std::make_pair(cost, it->first));
        }
    }
    machine_id_t primary = pick_n_best(primary_candidates, 1, primary_datacenter).front();
    sub_blueprint[primary] = blueprint_details::role_primary;

    for (std::map<datacenter_id_t, int>::const_iterator it = datacenter_affinities.begin(); it != datacenter_affinities.end(); it++) {
        std::multimap<float, machine_id_t> secondary_candidates;
        for (std::map<machine_id_t, datacenter_id_t>::const_iterator jt = machine_data_centers.begin();
                jt != machine_data_centers.end(); jt++) {
            if (jt->second == it->first && jt->first != primary) {
                float cost;
                if (std_contains(directory, jt->first)) {
                    cost = estimate_cost_to_get_up_to_date(directory.find(jt->first)->second, shard);
                } else {
                    cost = 3.0;
                }

                if (!std_contains(pinnings, it->first)) {
                    cost += 4.0;
                }

                secondary_candidates.insert(std::make_pair(cost, jt->first));
            }
        }
        std::vector<machine_id_t> secondaries = pick_n_best(secondary_candidates, it->second, it->first);
        for (std::vector<machine_id_t>::iterator jt = secondaries.begin(); jt != secondaries.end(); jt++) {
            sub_blueprint[*jt] = blueprint_details::role_secondary;
        }
    }

    for (std::map<machine_id_t, datacenter_id_t>::const_iterator it = machine_data_centers.begin();
            it != machine_data_centers.end(); it++) {
        /* This will set the value to `role_nothing` iff the peer is not already
        in the blueprint. */
        sub_blueprint.insert(std::make_pair(it->first, blueprint_details::role_nothing));
    }

    return sub_blueprint;
}

template<class protocol_t>
persistable_blueprint_t<protocol_t> suggest_blueprint(
        const std::map<machine_id_t, reactor_business_card_t<protocol_t> > &directory,
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &datacenter_affinities,
        const std::set<typename protocol_t::region_t> &shards,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const region_map_t<protocol_t, std::set<machine_id_t> > &pinnings) {

    typedef region_map_t<protocol_t, std::set<machine_id_t> > pinnings_map_t;

    persistable_blueprint_t<protocol_t> blueprint;
    for (typename std::set<typename protocol_t::region_t>::const_iterator it = shards.begin();
            it != shards.end(); it++) {
        std::set<machine_id_t> machines_shard_is_pinned_to;
        pinnings_map_t masked_map = pinnings.mask(*it);

        for (typename pinnings_map_t::iterator pit  = masked_map.begin();
                                               pit != masked_map.end();
                                               ++pit) {
            machines_shard_is_pinned_to.insert(pit->second.begin(), pit->second.end());
        }

        std::map<machine_id_t, typename blueprint_details::role_t> shard_blueprint =
            suggest_blueprint_for_shard(directory, primary_datacenter, datacenter_affinities, *it, machine_data_centers, machines_shard_is_pinned_to);
        for (typename std::map<machine_id_t, typename blueprint_details::role_t>::iterator jt = shard_blueprint.begin();
                jt != shard_blueprint.end(); jt++) {
            blueprint.machines_roles[jt->first][*it] = jt->second;
        }
    }
    return blueprint;
}

#endif /* CLUSTERING_SUGGESTER_SUGGESTER_TCC_ */
