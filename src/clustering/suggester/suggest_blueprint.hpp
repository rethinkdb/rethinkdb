// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_SUGGESTER_SUGGEST_BLUEPRINT_HPP_
#define CLUSTERING_SUGGESTER_SUGGEST_BLUEPRINT_HPP_

#include <map>
#include <set>
#include <string>

#include "clustering/administration/datacenter_metadata.hpp"
#include "clustering/administration/persistable_blueprint.hpp"
#include "clustering/reactor/metadata.hpp"

class nonoverlapping_regions_t;

class cannot_satisfy_goals_exc_t : public std::exception {
private:
    std::string desc;
public:
    explicit cannot_satisfy_goals_exc_t(const std::string &_desc)
        : desc(_desc)
    { }

    ~cannot_satisfy_goals_exc_t() throw() { }

    const char *what() const throw () {
        return desc.c_str();
    }

};

persistable_blueprint_t suggest_blueprint(
        const std::map<machine_id_t, reactor_business_card_t> &directory,
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &datacenter_affinities,
        const nonoverlapping_regions_t &shards,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const region_map_t<machine_id_t> &primary_pinnings,
        const region_map_t<std::set<machine_id_t> > &secondary_pinnings,
        std::map<machine_id_t, int> *usage,
        bool prioritize_distribution);

#endif /* CLUSTERING_SUGGESTER_SUGGEST_BLUEPRINT_HPP_ */
