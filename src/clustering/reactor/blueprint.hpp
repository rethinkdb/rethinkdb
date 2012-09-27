#ifndef CLUSTERING_REACTOR_BLUEPRINT_HPP_
#define CLUSTERING_REACTOR_BLUEPRINT_HPP_

#include <map>

#include "rpc/connectivity/connectivity.hpp"
#include "rpc/serialize_macros.hpp"

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

    void guarantee_valid() const THROWS_NOTHING;
    void add_peer(const peer_id_t &id);
    void add_role(const peer_id_t &id, const typename protocol_t::region_t &region, blueprint_role_t role);

    role_map_t peers_roles;
};

template <class protocol_t>
void debug_print(append_only_printf_buffer_t *buf, const blueprint_t<protocol_t> &blueprint);

#endif /* CLUSTERING_REACTOR_BLUEPRINT_HPP_ */

