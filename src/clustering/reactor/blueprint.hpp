#ifndef __CLUSTERING_REACTOR_BLUEPRINT_HPP__
#define __CLUSTERING_REACTOR_BLUEPRINT_HPP__

#include "utils.hpp"
#include <boost/optional.hpp>

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

template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(role_t *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(role_t *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, role_t *, const ctx_t &);

template <class ctx_t>
void on_subfield_change(role_t *, const ctx_t &);
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

        //calling this just to trigger some exceptions
        region_join(std::vector<typename protocol_t::region_t>(ref_regions.begin(), ref_regions.end()));

        for (typename role_map_t::const_iterator it =  peers_roles.begin();
                                                 it != peers_roles.end();
                                                 it++) {
            rassert(keys(it->second) == ref_regions, "Found blueprint with different peers having different sharding schemes.");
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

    bool operator==(const blueprint_t &other) const {
        return peers_roles == other.peers_roles;
    }

    RDB_MAKE_ME_SERIALIZABLE_1(peers_roles);
};

template <class protocol_t, class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(blueprint_t<protocol_t> *, const ctx_t &);

template <class protocol_t, class ctx_t>
cJSON *render_as_json(blueprint_t<protocol_t> *, const ctx_t &);

template <class protocol_t, class ctx_t>
void apply_json_to(cJSON *, blueprint_t<protocol_t> *, const ctx_t &);

template <class protocol_t, class ctx_t>
void on_subfield_change(blueprint_t<protocol_t> *, const ctx_t &);

#include "clustering/reactor/blueprint.tcc"

#endif /* __CLUSTERING_REACTOR_BLUEPRINT_HPP__ */

