// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_DATACENTER_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_DATACENTER_METADATA_HPP_

#include <map>
#include <string>

#include "containers/archive/stl_types.hpp"
#include "containers/name_string.hpp"
#include "containers/uuid.hpp"
#include "http/json.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "rpc/serialize_macros.hpp"


class datacenter_semilattice_metadata_t {
public:
    vclock_t<name_string_t> name;
    RDB_MAKE_ME_SERIALIZABLE_1(name);
};

void debug_print(printf_buffer_t *buf, const datacenter_semilattice_metadata_t &m);

RDB_MAKE_SEMILATTICE_JOINABLE_1(datacenter_semilattice_metadata_t, name);
RDB_MAKE_EQUALITY_COMPARABLE_1(datacenter_semilattice_metadata_t, name);

json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(datacenter_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
cJSON *with_ctx_render_as_json(datacenter_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_apply_json_to(cJSON *change, datacenter_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_on_subfield_change(datacenter_semilattice_metadata_t *, const vclock_ctx_t &);

class datacenters_semilattice_metadata_t {
public:
    typedef std::map<datacenter_id_t, deletable_t<datacenter_semilattice_metadata_t> > datacenter_map_t;
    datacenter_map_t datacenters;

    RDB_MAKE_ME_SERIALIZABLE_1(datacenters);
};

RDB_MAKE_SEMILATTICE_JOINABLE_1(datacenters_semilattice_metadata_t, datacenters);
RDB_MAKE_EQUALITY_COMPARABLE_1(datacenters_semilattice_metadata_t, datacenters);

//json adapter concept for datacenters_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(datacenters_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
cJSON *with_ctx_render_as_json(datacenters_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_apply_json_to(cJSON *change, datacenters_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_on_subfield_change(datacenters_semilattice_metadata_t *target, const vclock_ctx_t &ctx);


#endif /* CLUSTERING_ADMINISTRATION_DATACENTER_METADATA_HPP_ */
