#ifndef __CLUSTERING_ADMINISTRATION_MACHINE_METADATA_HPP__
#define __CLUSTERING_ADMINISTRATION_MACHINE_METADATA_HPP__

#include "utils.hpp"

#include <set>

#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/set.hpp"
#include "rpc/serialize_macros.hpp"

typedef boost::uuids::uuid machine_id_t;

class machines_semilattice_metadata_t {
public:
    std::set<machine_id_t> machines;

    RDB_MAKE_ME_SERIALIZABLE_1(machines);
};

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
    apply_as_directory(change, &target->machines, ctx);
}

template <class ctx_t>
void on_subfield_change(machines_semilattice_metadata_t *, const ctx_t &) { }

/* semilattice concept for machines_semilattice_metadata_t */
bool operator==(const machines_semilattice_metadata_t& a, const machines_semilattice_metadata_t& b);

void semilattice_join(machines_semilattice_metadata_t *a, const machines_semilattice_metadata_t &b);

#endif
