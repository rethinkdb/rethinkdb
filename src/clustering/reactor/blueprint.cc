// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/reactor/blueprint.hpp"

template <class protocol_t>
void blueprint_t<protocol_t>::guarantee_valid() const THROWS_NOTHING {
    if (peers_roles.empty()) {
        return; //any empty blueprint is valid
    }

    std::map<typename protocol_t::region_t, blueprint_role_t> ref_role_map = peers_roles.begin()->second;
    std::set<typename protocol_t::region_t> ref_regions = keys(ref_role_map);

    typename protocol_t::region_t join;
    guarantee(REGION_JOIN_OK == region_join(std::vector<typename protocol_t::region_t>(ref_regions.begin(), ref_regions.end()), &join));

    for (typename role_map_t::const_iterator it =  peers_roles.begin();
         it != peers_roles.end();
         it++) {
        guarantee(keys(it->second) == ref_regions, "Found blueprint with different peers having different sharding schemes.");
    }
}

template <class protocol_t>
void blueprint_t<protocol_t>::add_peer(const peer_id_t &id) {
    std::pair<typename std::map<peer_id_t, std::map<typename protocol_t::region_t, blueprint_role_t> >::iterator, bool>
        insert_res = peers_roles.insert(std::make_pair(id, std::map<typename protocol_t::region_t, blueprint_role_t>()));
    guarantee(insert_res.second);
}

template <class protocol_t>
void blueprint_t<protocol_t>::add_role(const peer_id_t &id, const typename protocol_t::region_t &region, blueprint_role_t role) {
    typename std::map<peer_id_t, std::map<typename protocol_t::region_t, blueprint_role_t> >::iterator it = peers_roles.find(id);
    guarantee(it != peers_roles.end());
    it->second.insert(std::make_pair(region, role));

    //TODO here we should assert that the range we just inserted does not
    //overlap any of the other ranges
}

template <class protocol_t>
void debug_print(append_only_printf_buffer_t *buf, const blueprint_t<protocol_t> &blueprint) {
    buf->appendf("blueprint{roles=");
    debug_print(buf, blueprint.peers_roles);
    buf->appendf("}");
}

#include "memcached/protocol.hpp"
template class blueprint_t<memcached_protocol_t>;
template void debug_print<memcached_protocol_t>(append_only_printf_buffer_t *buf, const blueprint_t<memcached_protocol_t> &blueprint);

#include "mock/dummy_protocol.hpp"
template class blueprint_t<mock::dummy_protocol_t>;
template void debug_print<mock::dummy_protocol_t>(append_only_printf_buffer_t *buf, const blueprint_t<mock::dummy_protocol_t> &blueprint);

#include "rdb_protocol/protocol.hpp"
template class blueprint_t<rdb_protocol_t>;
template void debug_print<rdb_protocol_t>(append_only_printf_buffer_t *buf, const blueprint_t<rdb_protocol_t> &blueprint);
