#include "clustering/administration/database_metadata.hpp"


json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(database_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["name"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_adapter_t<std::string>(&target->name, ctx));
    return res;
}

cJSON *with_ctx_render_as_json(database_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

void with_ctx_apply_json_to(cJSON *change, database_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

void with_ctx_on_subfield_change(database_semilattice_metadata_t *, const vclock_ctx_t &) { }


//json adapter concept for databases_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(databases_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return json_ctx_adapter_with_inserter_t<databases_semilattice_metadata_t::database_map_t, vclock_ctx_t>(&target->databases, generate_uuid, ctx).get_subfields();
}

cJSON *with_ctx_render_as_json(databases_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    return with_ctx_render_as_json(&target->databases, ctx);
}

void with_ctx_apply_json_to(cJSON *change, databases_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    apply_as_directory(change, &target->databases, ctx);
}

void with_ctx_on_subfield_change(databases_semilattice_metadata_t *target, const vclock_ctx_t &ctx) {
    with_ctx_on_subfield_change(&target->databases, ctx);
}

