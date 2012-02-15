#ifndef __CLUSTERING_REACTOR_BLUEPRINT_HPP__
#define __CLUSTERING_REACTOR_BLUEPRINT_HPP__

#include "utils.hpp"
#include <boost/optional.hpp>

#include "rpc/connectivity/connectivity.hpp"

template<class protocol_t>
class blueprint_t {
public:
    enum role_t {
        role_primary,
        role_secondary,
        role_nothing
    };

    void assert_valid() THROWS_NOTHING;

    void add_peer(const peer_id_t &id) {
        rassert(peers.find(id) == peers.end());
        peers[id];
    }

    void add_role(const peer_id_t &id, const typename protocol_t::region_t &region, role_t role) {
        rassert(peers.find(id) != peers.end());

        peers[id].insert(std::make_pair(region, role));

        //TODO here we should assert that the range we just inserted does not
        //overlap any of the other ranges
    }

    //TODO if we swap the region_t and peer_id_t's positions in these maps we
    //can get better data structure integrity. It might get a bit tricky
    //though.
    typedef std::map<typename protocol_t::region_t, role_t> region_to_role_map_t;

    typedef std::map<peer_id_t, region_to_role_map_t> role_map_t;
    role_map_t peers_roles;
};

#endif /* __CLUSTERING_REACTOR_BLUEPRINT_HPP__ */
