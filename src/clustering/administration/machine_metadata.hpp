#ifndef CLUSTERING_ADMINISTRATION_MACHINE_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_MACHINE_METADATA_HPP_

#include "utils.hpp"

#include <set>

#include "clustering/administration/datacenter_metadata.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "rpc/serialize_macros.hpp"

typedef boost::uuids::uuid machine_id_t;

class machine_semilattice_metadata_t {
public:
    vclock_t<datacenter_id_t> datacenter;
    vclock_t<std::string> name;
#ifndef NDEBUG
    vclock_t<int> port_offset;
#endif


#ifndef NDEBUG
    RDB_MAKE_ME_SERIALIZABLE_3(datacenter, name, port_offset);
#else
    RDB_MAKE_ME_SERIALIZABLE_2(datacenter, name);
#endif
};

#ifndef NDEBUG
RDB_MAKE_SEMILATTICE_JOINABLE_3(machine_semilattice_metadata_t, datacenter, name, port_offset);
RDB_MAKE_EQUALITY_COMPARABLE_3(machine_semilattice_metadata_t, datacenter, name, port_offset);
#else
RDB_MAKE_SEMILATTICE_JOINABLE_2(machine_semilattice_metadata_t, datacenter, name);
RDB_MAKE_EQUALITY_COMPARABLE_2(machine_semilattice_metadata_t, datacenter, name);
#endif

//json adapter concept for machine_semilattice_metadata_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(machine_semilattice_metadata_t *target, const ctx_t &) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    res["datacenter_uuid"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<vclock_t<datacenter_id_t>, ctx_t>(&target->datacenter));
    res["name"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<vclock_t<std::string>, ctx_t>(&target->name));

#ifndef NDEBUG
    res["port_offset"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<vclock_t<int>, ctx_t>(&target->port_offset));
#endif
    return res;
}

template <class ctx_t>
cJSON *render_as_json(machine_semilattice_metadata_t *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, machine_semilattice_metadata_t *target, const ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class ctx_t>
void on_subfield_change(machine_semilattice_metadata_t *, const ctx_t &) { }

class machines_semilattice_metadata_t {
public:
    typedef std::map<machine_id_t, deletable_t<machine_semilattice_metadata_t> > machine_map_t;
    machine_map_t machines;

    RDB_MAKE_ME_SERIALIZABLE_1(machines);
};

RDB_MAKE_SEMILATTICE_JOINABLE_1(machines_semilattice_metadata_t, machines);
RDB_MAKE_EQUALITY_COMPARABLE_1(machines_semilattice_metadata_t, machines);

//json adapter concept for machines_semilattice_metadata_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(machines_semilattice_metadata_t *target, const ctx_t &ctx) {
    return get_json_subfields(&target->machines, ctx);
}

template <class ctx_t>
cJSON *render_as_json(machines_semilattice_metadata_t *target, const ctx_t &ctx) {
    return render_as_json(&target->machines, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, machines_semilattice_metadata_t *target, const ctx_t &ctx) {
    apply_json_to(change, &target->machines, ctx);
}

template <class ctx_t>
void on_subfield_change(machines_semilattice_metadata_t *, const ctx_t &) { }

#endif
