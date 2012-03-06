#ifndef __CLUSTERING_ADMINISTRATION_DATACENTER_METADATA_HPP__
#define __CLUSTERING_ADMINISTRATION_DATACENTER_METADATA_HPP__

#include "errors.hpp"
#include <boost/uuid/uuid.hpp>

#include "rpc/serialize_macros.hpp"
#include "http/json.hpp"
#include "http/json/json_adapter.hpp"


typedef boost::uuids::uuid datacenter_id_t;

class datacenter_semilattice_metadata_t {
    RDB_MAKE_ME_SERIALIZABLE_0();
};

template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(datacenter_semilattice_metadata_t *, const ctx_t &) {
    return typename json_adapter_if_t<ctx_t>::json_adapter_map_t();
}

template <class ctx_t>
cJSON *render_as_json(datacenter_semilattice_metadata_t *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, datacenter_semilattice_metadata_t *target, const ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class ctx_t>
void on_subfield_change(datacenter_semilattice_metadata_t *, const ctx_t &) { }

/* semilattice concept for dataceneter_semilattice_metadata_t */
bool operator==(const datacenter_semilattice_metadata_t &a, const datacenter_semilattice_metadata_t &b);

void semilattice_join(datacenter_semilattice_metadata_t *a, const datacenter_semilattice_metadata_t &b);

#endif
