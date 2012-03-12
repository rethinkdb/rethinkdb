#ifndef __CLUSTERING_ADMINISTRATION_DATACENTER_METADATA_HPP__
#define __CLUSTERING_ADMINISTRATION_DATACENTER_METADATA_HPP__

#include "errors.hpp"
#include <boost/uuid/uuid.hpp>

#include "http/json.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "rpc/serialize_macros.hpp"


typedef boost::uuids::uuid datacenter_id_t;

class datacenter_semilattice_metadata_t {
public:
    vclock_t<std::string> name;
    RDB_MAKE_ME_SERIALIZABLE_1(name);
};

RDB_MAKE_SEMILATTICE_JOINABLE_1(datacenter_semilattice_metadata_t, name);
RDB_MAKE_EQUALITY_COMPARABLE_1(datacenter_semilattice_metadata_t, name);

template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(datacenter_semilattice_metadata_t *target, const ctx_t &) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    res["name"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<vclock_t<std::string>, ctx_t>(&target->name));
    return res;
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

#endif
