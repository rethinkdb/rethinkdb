#ifndef CLUSTERING_ADMINISTRATION_DATACENTER_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_DATACENTER_METADATA_HPP_

#include <map>
#include <string>

#include "clustering/administration/http/json_adapters.hpp"
#include "containers/uuid.hpp"
#include "http/json.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "rpc/serialize_macros.hpp"


typedef uuid_t datacenter_id_t;

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
    res["name"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_vclock_adapter_t<std::string, ctx_t>(&target->name));
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

class datacenters_semilattice_metadata_t {
public:
    typedef std::map<datacenter_id_t, deletable_t<datacenter_semilattice_metadata_t> > datacenter_map_t;
    datacenter_map_t datacenters;

    RDB_MAKE_ME_SERIALIZABLE_1(datacenters);
};

RDB_MAKE_SEMILATTICE_JOINABLE_1(datacenters_semilattice_metadata_t, datacenters);
RDB_MAKE_EQUALITY_COMPARABLE_1(datacenters_semilattice_metadata_t, datacenters);

//json adapter concept for datacenters_semilattice_metadata_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(datacenters_semilattice_metadata_t *target, const ctx_t &ctx) {
    return json_adapter_with_inserter_t<datacenters_semilattice_metadata_t::datacenter_map_t, ctx_t>(&target->datacenters, generate_uuid).get_subfields(ctx);
}

template <class ctx_t>
cJSON *render_as_json(datacenters_semilattice_metadata_t *target, const ctx_t &ctx) {
    return render_as_json(&target->datacenters, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, datacenters_semilattice_metadata_t *target, const ctx_t &ctx) {
    apply_as_directory(change, &target->datacenters, ctx);
}

template <class ctx_t>
void on_subfield_change(datacenters_semilattice_metadata_t *target, const ctx_t &ctx) {
    on_subfield_change(&target->datacenters, ctx);
}

#endif /* CLUSTERING_ADMINISTRATION_DATACENTER_METADATA_HPP_ */
