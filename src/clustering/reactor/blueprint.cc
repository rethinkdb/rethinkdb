// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/reactor/blueprint.hpp"

#include "debug.hpp"
#include "stl_utils.hpp"

void blueprint_t::guarantee_valid() const THROWS_NOTHING {
    if (peers_roles.empty()) {
        return; //any empty blueprint is valid
    }

    std::map<region_t, blueprint_role_t> ref_role_map = peers_roles.begin()->second;
    std::set<region_t> ref_regions = keys(ref_role_map);

    region_t join;
    guarantee(REGION_JOIN_OK == region_join(std::vector<region_t>(ref_regions.begin(), ref_regions.end()), &join));

    for (role_map_t::const_iterator it =  peers_roles.begin();
         it != peers_roles.end();
         it++) {
        guarantee(keys(it->second) == ref_regions, "Found blueprint with different peers having different sharding schemes.");
    }
}

void blueprint_t::add_peer(const peer_id_t &id) {
    std::pair<std::map<peer_id_t, std::map<region_t, blueprint_role_t> >::iterator, bool>
        insert_res = peers_roles.insert(std::make_pair(id, std::map<region_t, blueprint_role_t>()));
    guarantee(insert_res.second);
}

void blueprint_t::add_role(const peer_id_t &id, const region_t &region, blueprint_role_t role) {
    std::map<peer_id_t, std::map<region_t, blueprint_role_t> >::iterator it = peers_roles.find(id);
    guarantee(it != peers_roles.end());
    it->second.insert(std::make_pair(region, role));

    //TODO here we should assert that the range we just inserted does not
    //overlap any of the other ranges
}

void debug_print(printf_buffer_t *buf, const blueprint_t &blueprint) {
    buf->appendf("blueprint{roles=");
    debug_print(buf, blueprint.peers_roles);
    buf->appendf("}");
}

