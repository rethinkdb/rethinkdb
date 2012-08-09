#include "clustering/administration/machine_metadata.hpp"

//json adapter concept for machine_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(machine_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["datacenter_uuid"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<datacenter_id_t>(&target->datacenter, ctx));
    res["name"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<std::string>(&target->name, ctx));

    return res;
}

cJSON *with_ctx_render_as_json(machine_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

void with_ctx_apply_json_to(cJSON *change, machine_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

void with_ctx_on_subfield_change(machine_semilattice_metadata_t *, const vclock_ctx_t &) { }



//json adapter concept for machines_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(machines_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return with_ctx_get_json_subfields(&target->machines, ctx);
}

cJSON *with_ctx_render_as_json(machines_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return with_ctx_render_as_json(&target->machines, ctx);
}

void with_ctx_apply_json_to(cJSON *change, machines_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    with_ctx_apply_json_to(change, &target->machines, ctx);
}

void with_ctx_on_subfield_change(machines_semilattice_metadata_t *, const vclock_ctx_t &) { }

