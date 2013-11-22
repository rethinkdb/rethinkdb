// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSISTABLE_BLUEPRINT_HPP_
#define CLUSTERING_ADMINISTRATION_PERSISTABLE_BLUEPRINT_HPP_

#include <map>

#include "clustering/reactor/blueprint.hpp"
#include "clustering/administration/machine_metadata.hpp"


/* This is like `blueprint_t`, except that it is indexed by `machine_id_t`
instead of `peer_id_t`. This is important because peer IDs chan change when a
node restarts, but machine IDs do not. So data structures that contain peer IDs,
such as `blueprint_t`, should not be persisted. */

json_adapter_if_t::json_adapter_map_t get_json_subfields(blueprint_role_t *);
cJSON *render_as_json(blueprint_role_t *);
void apply_json_to(cJSON *, blueprint_role_t *);

template<class protocol_t>
class persistable_blueprint_t {
public:
    //TODO if we swap the region_t and peer_id_t's positions in these maps we
    //can get better data structure integrity. It might get a bit tricky
    //though.

    typedef std::map<typename protocol_t::region_t, blueprint_role_t> region_to_role_map_t;
    typedef std::map<machine_id_t, region_to_role_map_t> role_map_t;

    role_map_t machines_roles;

    typedef std::map<typename protocol_t::region_t, bool> failover_map_t;
    failover_map_t failover;

    RDB_MAKE_ME_SERIALIZABLE_2(machines_roles, failover);
};

template<class protocol_t>
RDB_MAKE_EQUALITY_COMPARABLE_2(persistable_blueprint_t<protocol_t>,
        machines_roles, failover);

template <class protocol_t>
void debug_print(printf_buffer_t *buf, const persistable_blueprint_t<protocol_t> &x);


template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(persistable_blueprint_t<protocol_t> *);

template <class protocol_t>
cJSON *render_as_json(persistable_blueprint_t<protocol_t> *);

template <class protocol_t>
void apply_json_to(cJSON *, persistable_blueprint_t<protocol_t> *);

#endif /* CLUSTERING_ADMINISTRATION_PERSISTABLE_BLUEPRINT_HPP_ */
