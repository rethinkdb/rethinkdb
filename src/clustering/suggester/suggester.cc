#include "clustering/suggester/suggester.hpp"

#include "stl_utils.hpp"
#include "containers/priority_queue.hpp"

namespace {

struct priority_t {
    machine_id_t machine_id;

    bool pinned;
    bool would_rob_secondary;
    int redundancy_cost;
    float backfill_cost;

    priority_t() { }  // TODO: fix priority_queue_t

    priority_t(machine_id_t _machine_id, bool _pinned, bool _would_rob_secondary,
               int _redundancy_cost, float _backfill_cost)
        : machine_id(_machine_id), pinned(_pinned), would_rob_secondary(_would_rob_secondary),
          redundancy_cost(_redundancy_cost), backfill_cost(_backfill_cost)
    { }
};

//It's easier to think about this function if you imagine it as the answer to:
//is y of higher priority than x

bool operator<(const priority_t &x, const priority_t &y) {
//CONTINUE:
    //Round 1: Pinnings.
    //F I G H T
    if (x.pinned && !y.pinned) {
        return false;
    } else if (y.pinned && !x.pinned) {
        return true;
    }

    //Round 2: Would Rob Secondary.
    //F I G H T
    if (x.would_rob_secondary && !y.would_rob_secondary) {
        return true;
    } else if (y.would_rob_secondary && !x.would_rob_secondary) {
        return false;
    }

    //Round 3: Redundancy.
    //F I G H T
    if (x.redundancy_cost < y.redundancy_cost) {
        return false;
    } else if (y.redundancy_cost < x.redundancy_cost) {
        return true;
    }

    //Round 4: Backfill cost.
    //F I G H T
    if (x.backfill_cost < y.backfill_cost) {
        return false;
    } else if (y.backfill_cost < x.backfill_cost) {
        return true;
    }

    //You Lose!!
    //Continue?
    //(uncomment the below line to continue)
    //goto CONTINUE;
    //5
    //4
    //3
    //2
    //1
    //Last chance:
    //goto CONTINUE;
    //Didn't continue: Game Over
    return false; //They're equal
}


} //anonymous namespace

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

std::vector<machine_id_t> pick_n_best(priority_queue_t<priority_t> *candidates, int n, const datacenter_id_t &datacenter) {
    std::vector<machine_id_t> result;
    while (result.size() < static_cast<size_t>(n)) {
        if (candidates->empty()) {
            throw cannot_satisfy_goals_exc_t(strprintf("Didn't have enough unused machines in datacenter %s, we needed %d\n", uuid_to_str(datacenter).c_str(), n));
        } else {
            result.push_back(candidates->pop().machine_id);
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
        const std::set<machine_id_t> &primary_pinnings,
        const std::set<machine_id_t> &secondary_pinnings,
        std::map<machine_id_t, int> *primary_usage,
        std::map<machine_id_t, int> *secondary_usage) {

    std::map<machine_id_t, typename blueprint_details::role_t> sub_blueprint;

    priority_queue_t<priority_t> primary_candidates;
    for (std::map<machine_id_t, datacenter_id_t>::const_iterator it = machine_data_centers.begin();
            it != machine_data_centers.end(); it++) {
        if (it->second == primary_datacenter) {
            bool pinned = std_contains(primary_pinnings, it->first);

            bool would_rob_secondary = std_contains(secondary_pinnings, it->first);

            int redundancy_cost = get_with_default(*primary_usage, it->first, 0);

            float backfill_cost;
            if (std_contains(directory, it->first)) {
                backfill_cost = estimate_cost_to_get_up_to_date(directory.find(it->first)->second, shard);
            } else {
                backfill_cost = 3.0;
            }

            primary_candidates.push(priority_t(it->first, pinned, would_rob_secondary, redundancy_cost, backfill_cost));
        }
    }
    machine_id_t primary = pick_n_best(&primary_candidates, 1, primary_datacenter).front();
    sub_blueprint[primary] = blueprint_details::role_primary;

    //Update primary_usage
    get_with_default(*primary_usage, primary, 0)++;

    for (std::map<datacenter_id_t, int>::const_iterator it = datacenter_affinities.begin(); it != datacenter_affinities.end(); it++) {
        priority_queue_t<priority_t> secondary_candidates;
        for (std::map<machine_id_t, datacenter_id_t>::const_iterator jt = machine_data_centers.begin();
                jt != machine_data_centers.end(); jt++) {
            if (jt->second == it->first && jt->first != primary) {
                bool pinned = std_contains(secondary_pinnings, jt->first);

                int redundancy_cost = get_with_default(*secondary_usage, jt->first, 0);

                bool would_rob_secondary = false; //we're the secondary, we shan't be robbing ourselves

                float backfill_cost;
                if (std_contains(directory, jt->first)) {
                    backfill_cost = estimate_cost_to_get_up_to_date(directory.find(jt->first)->second, shard);
                } else {
                    backfill_cost = 3.0;
                }

                secondary_candidates.push(priority_t(jt->first, pinned, would_rob_secondary, redundancy_cost, backfill_cost));
            }
        }
        std::vector<machine_id_t> secondaries = pick_n_best(&secondary_candidates, it->second, it->first);
        for (std::vector<machine_id_t>::iterator jt = secondaries.begin(); jt != secondaries.end(); jt++) {
            //Update secondary usage
            get_with_default(*secondary_usage, *jt, 0)++;
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
        const region_map_t<protocol_t, machine_id_t> &primary_pinnings,
        const region_map_t<protocol_t, std::set<machine_id_t> > &secondary_pinnings) {

    typedef region_map_t<protocol_t, machine_id_t> primary_pinnings_map_t;
    typedef region_map_t<protocol_t, std::set<machine_id_t> > secondary_pinnings_map_t;

    persistable_blueprint_t<protocol_t> blueprint;

    //Maps to keep track of how much we're using each machine
    std::map<machine_id_t, int> primary_usage, secondary_usage;
    for (typename std::set<typename protocol_t::region_t>::const_iterator it = shards.begin();
            it != shards.end(); it++) {
        std::set<machine_id_t> machines_shard_primary_is_pinned_to;
        primary_pinnings_map_t primary_masked_map = primary_pinnings.mask(*it);

        for (typename primary_pinnings_map_t::iterator pit  = primary_masked_map.begin();
                                                       pit != primary_masked_map.end();
                                                       ++pit) {
            machines_shard_primary_is_pinned_to.insert(pit->second);
        }

        std::set<machine_id_t> machines_shard_secondary_is_pinned_to;
        secondary_pinnings_map_t secondary_masked_map = secondary_pinnings.mask(*it);

        for (typename secondary_pinnings_map_t::iterator pit  = secondary_masked_map.begin();
                                                         pit != secondary_masked_map.end();
                                                         ++pit) {
            machines_shard_secondary_is_pinned_to.insert(pit->second.begin(), pit->second.end());
        }

        std::map<machine_id_t, typename blueprint_details::role_t> shard_blueprint =
            suggest_blueprint_for_shard(directory, primary_datacenter, datacenter_affinities, *it,
                                        machine_data_centers, machines_shard_primary_is_pinned_to,
                                        machines_shard_secondary_is_pinned_to, &primary_usage,
                                        &secondary_usage);
        for (typename std::map<machine_id_t, typename blueprint_details::role_t>::iterator jt = shard_blueprint.begin();
                jt != shard_blueprint.end(); jt++) {
            blueprint.machines_roles[jt->first][*it] = jt->second;
        }
    }

    return blueprint;
}

#include "mock/dummy_protocol.hpp"


template
persistable_blueprint_t<mock::dummy_protocol_t> suggest_blueprint<mock::dummy_protocol_t>(
        const std::map<machine_id_t, reactor_business_card_t<mock::dummy_protocol_t> > &directory,
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &datacenter_affinities,
        const std::set<mock::dummy_protocol_t::region_t> &shards,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const region_map_t<mock::dummy_protocol_t, machine_id_t> &primary_pinnings,
        const region_map_t<mock::dummy_protocol_t, std::set<machine_id_t> > &secondary_pinnings);


#include "memcached/protocol.hpp"

template
persistable_blueprint_t<memcached_protocol_t> suggest_blueprint<memcached_protocol_t>(
        const std::map<machine_id_t, reactor_business_card_t<memcached_protocol_t> > &directory,
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &datacenter_affinities,
        const std::set<memcached_protocol_t::region_t> &shards,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const region_map_t<memcached_protocol_t, machine_id_t> &primary_pinnings,
        const region_map_t<memcached_protocol_t, std::set<machine_id_t> > &secondary_pinnings);


#include "rdb_protocol/protocol.hpp"

template
persistable_blueprint_t<rdb_protocol_t> suggest_blueprint<rdb_protocol_t>(
        const std::map<machine_id_t, reactor_business_card_t<rdb_protocol_t> > &directory,
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &datacenter_affinities,
        const std::set<rdb_protocol_t::region_t> &shards,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const region_map_t<rdb_protocol_t, machine_id_t> &primary_pinnings,
        const region_map_t<rdb_protocol_t, std::set<machine_id_t> > &secondary_pinnings);
