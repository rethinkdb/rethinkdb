#include "clustering/administration/datacenter_metadata.hpp"

//json adapter concept for datacenter_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(datacenter_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["name"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<std::string>(&target->name, ctx));
    return res;
}

cJSON *with_ctx_render_as_json(datacenter_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

void with_ctx_apply_json_to(cJSON *change, datacenter_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

void with_ctx_on_subfield_change(datacenter_semilattice_metadata_t *, const vclock_ctx_t &) { }



//json adapter concept for datacenters_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(datacenters_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return json_ctx_adapter_with_inserter_t<datacenters_semilattice_metadata_t::datacenter_map_t, vclock_ctx_t>(&target->datacenters, generate_uuid, ctx).get_subfields();
}

cJSON *with_ctx_render_as_json(datacenters_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return with_ctx_render_as_json(&target->datacenters, ctx);
}

void with_ctx_apply_json_to(cJSON *change, datacenters_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, &target->datacenters, ctx);
}

void with_ctx_on_subfield_change(datacenters_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    with_ctx_on_subfield_change(&target->datacenters, ctx);
}
