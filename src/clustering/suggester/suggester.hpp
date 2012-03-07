#ifndef __CLUSTERING_SUGGESTER_SUGGESTER_HPP__
#define __CLUSTERING_SUGGESTER_SUGGESTER_HPP__

#include "clustering/administration/datacenter_metadata.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/metadata.hpp"

class cannot_satisfy_goals_exc_t : public std::exception {
public:
    const char *what() const throw () {
        return "The given goals are impossible to satisfy with the given "
            "machines.";
    }
};

template<class protocol_t>
blueprint_t<protocol_t> suggest_blueprint(
        const std::map<peer_id_t, reactor_business_card_t<protocol_t> > &directory,
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &datacenter_affinities,
        const std::set<typename protocol_t::region_t> &shards,
        const std::map<peer_id_t, datacenter_id_t> &machine_data_centers);

#include "clustering/suggester/suggester.tcc"

#endif /* __CLUSTERING_SUGGESTER_SUGGESTER_HPP__ */
