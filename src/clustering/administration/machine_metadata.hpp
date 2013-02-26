// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MACHINE_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_MACHINE_METADATA_HPP_

#include <map>
#include <set>
#include <string>

#include "clustering/administration/datacenter_metadata.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "rpc/serialize_macros.hpp"
#include "utils.hpp"

typedef uuid_u machine_id_t;

class machine_semilattice_metadata_t {
public:
    vclock_t<datacenter_id_t> datacenter;
    vclock_t<name_string_t> name;


    RDB_MAKE_ME_SERIALIZABLE_2(datacenter, name);
};

RDB_MAKE_SEMILATTICE_JOINABLE_2(machine_semilattice_metadata_t, datacenter, name);
RDB_MAKE_EQUALITY_COMPARABLE_2(machine_semilattice_metadata_t, datacenter, name);

//json adapter concept for machine_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(machine_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
cJSON *with_ctx_render_as_json(machine_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_apply_json_to(cJSON *change, machine_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_on_subfield_change(machine_semilattice_metadata_t *, const vclock_ctx_t &);

class machines_semilattice_metadata_t {
public:
    typedef std::map<machine_id_t, deletable_t<machine_semilattice_metadata_t> > machine_map_t;
    machine_map_t machines;

    RDB_MAKE_ME_SERIALIZABLE_1(machines);
};

RDB_MAKE_SEMILATTICE_JOINABLE_1(machines_semilattice_metadata_t, machines);
RDB_MAKE_EQUALITY_COMPARABLE_1(machines_semilattice_metadata_t, machines);

//json adapter concept for machines_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(machines_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
cJSON *with_ctx_render_as_json(machines_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_apply_json_to(cJSON *change, machines_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_on_subfield_change(machines_semilattice_metadata_t *, const vclock_ctx_t &);

#endif /* CLUSTERING_ADMINISTRATION_MACHINE_METADATA_HPP_ */
