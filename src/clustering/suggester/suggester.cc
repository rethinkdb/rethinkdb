#include "clustering/suggester/suggester.hpp"

#include "stl_utils.hpp"
#include "containers/priority_queue.hpp"
#include "clustering/generic/nonoverlapping_regions.hpp"

namespace {

struct priority_t {
    machine_id_t machine_id;

    bool pinned;
    bool would_rob_secondary;
    int redundancy_cost;
    double backfill_cost;
    bool prioritize_distribution;

    priority_t() { }  // TODO: fix priority_queue_t

    priority_t(machine_id_t _machine_id, bool _pinned, bool _would_rob_secondary,
               int _redundancy_cost, double _backfill_cost,
               bool _prioritize_distribution)
        : machine_id(_machine_id), pinned(_pinned), would_rob_secondary(_would_rob_secondary),
          redundancy_cost(_redundancy_cost), backfill_cost(_backfill_cost),
          prioritize_distribution(_prioritize_distribution)
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

    if (x.prioritize_distribution) {
        //Makes sure no one is doing anything insane
        guarantee(y.prioritize_distribution);

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
    } else {
        //Makes sure no one is doing anything insane
        guarantee(!y.prioritize_distribution);

        //Round 3: Backfill cost.
        //F I G H T
        if (x.backfill_cost < y.backfill_cost) {
            return false;
        } else if (y.backfill_cost < x.backfill_cost) {
            return true;
        }

        //Round 4: Redundancy.
        //F I G H T
        if (x.redundancy_cost < y.redundancy_cost) {
            return false;
        } else if (y.redundancy_cost < x.redundancy_cost) {
            return true;
        }
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
double estimate_cost_to_get_up_to_date(
        const reactor_business_card_t<protocol_t> &business_card,
        const typename protocol_t::region_t &shard) {
    typedef reactor_business_card_t<protocol_t> rb_t;
    region_map_t<protocol_t, double> costs(shard, 3);
    for (typename rb_t::activity_map_t::const_iterator it = business_card.activities.begin();
            it != business_card.activities.end(); it++) {
        typename protocol_t::region_t intersection = region_intersection(it->second.region, shard);
        if (!region_is_empty(intersection)) {
            int cost;
            if (boost::get<typename rb_t::primary_when_safe_t>(&it->second.activity)) {
                cost = 0;
            } else if (boost::get<typename rb_t::primary_t>(&it->second.activity)) {
                cost = 0;
            } else if (boost::get<typename rb_t::secondary_up_to_date_t>(&it->second.activity)) {
                cost = 1;
            } else if (boost::get<typename rb_t::secondary_without_primary_t>(&it->second.activity)) {
                cost = 2;
            } else if (boost::get<typename rb_t::secondary_backfilling_t>(&it->second.activity)) {
                cost = 2;
            } else if (boost::get<typename rb_t::nothing_when_safe_t>(&it->second.activity)) {
                cost = 3;
            } else if (boost::get<typename rb_t::nothing_when_done_erasing_t>(&it->second.activity)) {
                cost = 3;
            } else if (boost::get<typename rb_t::nothing_t>(&it->second.activity)) {
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
    double sum = 0;
    int count = 0;
    for (typename region_map_t<protocol_t, double>::iterator it = costs.begin(); it != costs.end(); it++) {
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

template <class protocol_t>
priority_t priority_for_machine(machine_id_t id, const std::set<machine_id_t> &positive_pinnings,
                                const std::set<machine_id_t> &negative_pinnings, const std::map<machine_id_t, int> &usage,
                                const std::map<machine_id_t, reactor_business_card_t<protocol_t> > &directory,
                                const typename protocol_t::region_t &shard,
                                bool prioritize_distribution) {
    bool pinned = std_contains(positive_pinnings, id);
    bool would_rob_someone = std_contains(negative_pinnings, id);
    int redundancy_cost = const_get_with_default(usage, id, 0);
    double backfill_cost;
    if (std_contains(directory, id)) {
        backfill_cost = estimate_cost_to_get_up_to_date(directory.find(id)->second, shard);
    } else {
        backfill_cost = 3.0;
    }

    return priority_t(id, pinned, would_rob_someone, redundancy_cost, backfill_cost, prioritize_distribution);
}

template<class protocol_t>
std::map<machine_id_t, blueprint_role_t> suggest_blueprint_for_shard(
        const std::map<machine_id_t, reactor_business_card_t<protocol_t> > &directory,
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &datacenter_affinities,
        const typename protocol_t::region_t &shard,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const std::set<machine_id_t> &primary_pinnings,
        const std::set<machine_id_t> &secondary_pinnings,
        std::map<machine_id_t, int> *usage,
        bool prioritize_distribution) {

    std::map<machine_id_t, blueprint_role_t> sub_blueprint;

    /* A set containing the machines currently not doing anything for this
     * shard. */
    std::set<machine_id_t> unused_machines;

    for (std::map<machine_id_t, datacenter_id_t>::const_iterator it  = machine_data_centers.begin();
                                                                 it != machine_data_centers.end();
                                                                 ++it) {
        unused_machines.insert(it->first);
    }

    /* Pick primary if we have a concretely assigned datacenter. */
    if (!primary_datacenter.is_nil()) {
        priority_queue_t<priority_t> primary_candidates;

        for (std::set<machine_id_t>::const_iterator mt  = unused_machines.begin();
                                                    mt != unused_machines.end();
                                                    ++mt) {
            if (machine_data_centers.find(*mt)->second == primary_datacenter) {
                primary_candidates.push(priority_for_machine(*mt, primary_pinnings, secondary_pinnings, *usage, directory, shard, prioritize_distribution));
            }
        }

        machine_id_t primary = pick_n_best(&primary_candidates, 1, primary_datacenter).front();
        unused_machines.erase(primary);

        sub_blueprint[primary] = blueprint_role_primary;

        //Update primary_usage
        get_with_default(*usage, primary, 0)++;
    }


    /* Pick secondaries for concretely assigned secondaries. */
    for (std::map<datacenter_id_t, int>::const_iterator it = datacenter_affinities.begin(); it != datacenter_affinities.end(); it++) {
        if (it->first.is_nil()) {
            continue; //We do the nil datacenter last
        }
        priority_queue_t<priority_t> secondary_candidates;
        for (std::set<machine_id_t>::const_iterator mt  = unused_machines.begin();
                                                    mt != unused_machines.end();
                                                    ++mt) {
            if (machine_data_centers.find(*mt)->second == it->first) {
                secondary_candidates.push(priority_for_machine(*mt, secondary_pinnings, primary_pinnings, *usage, directory, shard, prioritize_distribution));
            }
        }
        std::vector<machine_id_t> secondaries = pick_n_best(&secondary_candidates, it->second, it->first);

        for (std::vector<machine_id_t>::iterator jt = secondaries.begin(); jt != secondaries.end(); jt++) {
            //Update secondary usage
            get_with_default(*usage, *jt, 0)++;
            sub_blueprint[*jt] = blueprint_role_secondary;
            unused_machines.erase(*jt);
        }
    }

    /* Pick a primary if the primary datacenter is nil. */
    if (primary_datacenter.is_nil()) {
        priority_queue_t<priority_t> primary_candidates;

        for (std::set<machine_id_t>::const_iterator mt  = unused_machines.begin();
                                                    mt != unused_machines.end();
                                                    ++mt) {
            primary_candidates.push(priority_for_machine(*mt, primary_pinnings, secondary_pinnings, *usage, directory, shard, prioritize_distribution));
        }

        machine_id_t primary = pick_n_best(&primary_candidates, 1, primary_datacenter).front();
        unused_machines.erase(primary);

        sub_blueprint[primary] = blueprint_role_primary;

        //Update primary_usage
        get_with_default(*usage, primary, 0)++;
    }

    /* Finally pick the secondaries for the nil datacenter */
    if (std_contains(datacenter_affinities, nil_uuid())) {
        priority_queue_t<priority_t> secondary_candidates;
        for (std::set<machine_id_t>::const_iterator mt  = unused_machines.begin();
                                                    mt != unused_machines.end();
                                                    ++mt) {
            secondary_candidates.push(priority_for_machine(*mt, secondary_pinnings, primary_pinnings, *usage, directory, shard, prioritize_distribution));
        }
        std::vector<machine_id_t> secondaries = pick_n_best(&secondary_candidates, datacenter_affinities.find(nil_uuid())->second, nil_uuid());

        for (std::vector<machine_id_t>::iterator jt = secondaries.begin(); jt != secondaries.end(); jt++) {
            //Update secondary usage
            get_with_default(*usage, *jt, 0)++;
            sub_blueprint[*jt] = blueprint_role_secondary;
            unused_machines.erase(*jt);
        }
    }

    for (std::map<machine_id_t, datacenter_id_t>::const_iterator it = machine_data_centers.begin();
            it != machine_data_centers.end(); it++) {
        /* This will set the value to `role_nothing` iff the peer is not already
        in the blueprint. */
        sub_blueprint.insert(std::make_pair(it->first, blueprint_role_nothing));
    }

    return sub_blueprint;
}

template<class protocol_t>
persistable_blueprint_t<protocol_t> suggest_blueprint(
        const std::map<machine_id_t, reactor_business_card_t<protocol_t> > &directory,
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &datacenter_affinities,
        const nonoverlapping_regions_t<protocol_t> &shards,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const region_map_t<protocol_t, machine_id_t> &primary_pinnings,
        const region_map_t<protocol_t, std::set<machine_id_t> > &secondary_pinnings,
        std::map<machine_id_t, int> *usage,
        bool prioritize_distribution) {

    typedef region_map_t<protocol_t, machine_id_t> primary_pinnings_map_t;
    typedef region_map_t<protocol_t, std::set<machine_id_t> > secondary_pinnings_map_t;

    persistable_blueprint_t<protocol_t> blueprint;

    for (typename nonoverlapping_regions_t<protocol_t>::iterator it = shards.begin();
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

        std::map<machine_id_t, blueprint_role_t> shard_blueprint =
            suggest_blueprint_for_shard(directory, primary_datacenter, datacenter_affinities, *it,
                                        machine_data_centers, machines_shard_primary_is_pinned_to,
                                        machines_shard_secondary_is_pinned_to, usage, prioritize_distribution);
        for (typename std::map<machine_id_t, blueprint_role_t>::iterator jt = shard_blueprint.begin();
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
        const nonoverlapping_regions_t<mock::dummy_protocol_t> &shards,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const region_map_t<mock::dummy_protocol_t, machine_id_t> &primary_pinnings,
        const region_map_t<mock::dummy_protocol_t, std::set<machine_id_t> > &secondary_pinnings,
        std::map<machine_id_t, int> *usage,
        bool prioritize_distribution);


#include "memcached/protocol.hpp"

template
persistable_blueprint_t<memcached_protocol_t> suggest_blueprint<memcached_protocol_t>(
        const std::map<machine_id_t, reactor_business_card_t<memcached_protocol_t> > &directory,
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &datacenter_affinities,
        const nonoverlapping_regions_t<memcached_protocol_t> &shards,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const region_map_t<memcached_protocol_t, machine_id_t> &primary_pinnings,
        const region_map_t<memcached_protocol_t, std::set<machine_id_t> > &secondary_pinnings,
        std::map<machine_id_t, int> *usage,
        bool prioritize_distribution);


#include "rdb_protocol/protocol.hpp"

template
persistable_blueprint_t<rdb_protocol_t> suggest_blueprint<rdb_protocol_t>(
        const std::map<machine_id_t, reactor_business_card_t<rdb_protocol_t> > &directory,
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &datacenter_affinities,
        const nonoverlapping_regions_t<rdb_protocol_t> &shards,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const region_map_t<rdb_protocol_t, machine_id_t> &primary_pinnings,
        const region_map_t<rdb_protocol_t, std::set<machine_id_t> > &secondary_pinnings,
        std::map<machine_id_t, int> *usage,
        bool prioritize_distribution);
