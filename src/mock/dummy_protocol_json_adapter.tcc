#ifndef __MOCK_DUMMY_PROTOCOL_JSON_ADAPTER_TCC__
#define __MOCK_DUMMY_PROTOCOL_JSON_ADAPTER_TCC__

#include "http/json/json_adapter.hpp"

namespace mock {
//json adapter concept for dummy_protocol_t::region_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(dummy_protocol_t::region_t *, const ctx_t &) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t();
}

template <class ctx_t>
cJSON *render_as_json(dummy_protocol_t::region_t *target, const ctx_t &ctx) {
    return render_as_json(&target->keys, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, dummy_protocol_t::region_t *target, const ctx_t &ctx) {
#ifdef JSON_SHORTCUTS
    try {
        std::string region_spec = get_string(change);
        if (region_spec[1] != '-' || region_spec.size() != 3) {
            throw schema_mismatch_exc_t("Invalid region shortcut\n");
        }
        *target = dummy_protocol_t::region_t(region_spec[0], region_spec[2]);
    } catch (schema_mismatch_exc_t) {
        //do nothing
    }
#endif
    apply_json_to(change, &target->keys, ctx);
}

template <class ctx_t>
void  on_subfield_change(dummy_protocol_t::region_t *, const ctx_t &) { }
}//namespace mock 

#endif
