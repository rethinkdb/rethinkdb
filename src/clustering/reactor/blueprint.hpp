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

enum blueprint_role_t { blueprint_role_primary, blueprint_role_secondary, blueprint_role_nothing };
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(blueprint_role_t, int8_t, blueprint_role_primary, blueprint_role_nothing);

// Explain what a blueprint_t is here please.

template <class protocol_t>
class blueprint_t {
public:
    //TODO if we swap the region_t and peer_id_t's positions in these maps we
    //can get better data structure integrity. It might get a bit tricky
    //though.

    typedef std::map<typename protocol_t::region_t, blueprint_role_t> region_to_role_map_t;
    typedef std::map<peer_id_t, std::map<typename protocol_t::region_t, blueprint_role_t> > role_map_t;

    void assert_valid_unreviewed() const {
        return assert_valid_reviewed();
    }

    void assert_valid_reviewed() const THROWS_NOTHING {
        if (peers_roles.empty()) {
            return; //any empty blueprint is valid
        }

        std::map<typename protocol_t::region_t, blueprint_role_t> ref_role_map = peers_roles.begin()->second;
        std::set<typename protocol_t::region_t> ref_regions = keys(ref_role_map);

        typename protocol_t::region_t join;
        rassert_reviewed(REGION_JOIN_OK == region_join(std::vector<typename protocol_t::region_t>(ref_regions.begin(), ref_regions.end()), &join));

        for (typename role_map_t::const_iterator it =  peers_roles.begin();
                                                 it != peers_roles.end();
                                                 it++) {
            rassert_reviewed(keys(it->second) == ref_regions, "Found blueprint with different peers having different sharding schemes.");
        }
    }

    void add_peer(const peer_id_t &id) {
        std::pair<typename std::map<peer_id_t, std::map<typename protocol_t::region_t, blueprint_role_t> >::iterator, bool>
            insert_res = peers_roles.insert(std::make_pair(id, std::map<typename protocol_t::region_t, blueprint_role_t>()));
        guarantee_reviewed(insert_res.second);
    }

    void add_role(const peer_id_t &id, const typename protocol_t::region_t &region, blueprint_role_t role) {
        typename std::map<peer_id_t, std::map<typename protocol_t::region_t, blueprint_role_t> >::iterator it = peers_roles.find(id);
        guarantee_reviewed(it != peers_roles.end());
        it->second.insert(std::make_pair(region, role));

        //TODO here we should assert that the range we just inserted does not
        //overlap any of the other ranges
    }

    role_map_t peers_roles;
};

template <class protocol_t>
void debug_print(append_only_printf_buffer_t *buf, const blueprint_t<protocol_t> &blueprint) {
    buf->appendf("blueprint{roles=");
    debug_print(buf, blueprint.peers_roles);
    buf->appendf("}");
}

#endif /* CLUSTERING_REACTOR_BLUEPRINT_HPP_ */

