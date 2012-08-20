#ifndef CLUSTERING_REACTOR_BLUEPRINT_HPP_
#define CLUSTERING_REACTOR_BLUEPRINT_HPP_

#include <map>
#include <set>
#include <vector>

#include "containers/archive/archive.hpp"
#include "rpc/connectivity/connectivity.hpp"
#include "rpc/serialize_macros.hpp"
#include "stl_utils.hpp"
#include "http/json/json_adapter.hpp"

namespace blueprint_details {
enum role_t {
    role_primary,
    role_secondary,
    role_nothing
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(role_t, int8_t, role_primary, role_nothing);

} //namespace blueprint_details

template<class protocol_t>
class blueprint_t {
public:
    //TODO if we swap the region_t and peer_id_t's positions in these maps we
    //can get better data structure integrity. It might get a bit tricky
    //though.

    typedef std::map<typename protocol_t::region_t, blueprint_details::role_t> region_to_role_map_t;
    typedef std::map<peer_id_t, region_to_role_map_t> role_map_t;

    void assert_valid() const THROWS_NOTHING {
        if (peers_roles.empty()) {
            return; //any empty blueprint is valid
        }

        region_to_role_map_t ref_role_map = peers_roles.begin()->second;
        std::set<typename protocol_t::region_t> ref_regions = keys(ref_role_map);

        typename protocol_t::region_t join;
        rassert(REGION_JOIN_OK == region_join(std::vector<typename protocol_t::region_t>(ref_regions.begin(), ref_regions.end()), &join));

        for (typename role_map_t::const_iterator it =  peers_roles.begin();
                                                 it != peers_roles.end();
                                                 it++) {
            rassertf(keys(it->second) == ref_regions, "Found blueprint with different peers having different sharding schemes.");
        }
    }

    void add_peer(const peer_id_t &id) {
        rassert(peers_roles.find(id) == peers_roles.end());
        peers_roles[id] = region_to_role_map_t();
    }

    void add_role(const peer_id_t &id, const typename protocol_t::region_t &region, blueprint_details::role_t role) {
        rassert(peers_roles.find(id) != peers_roles.end());

        peers_roles[id].insert(std::make_pair(region, role));

        //TODO here we should assert that the range we just inserted does not
        //overlap any of the other ranges
    }

    role_map_t peers_roles;
};

#endif /* CLUSTERING_REACTOR_BLUEPRINT_HPP_ */

