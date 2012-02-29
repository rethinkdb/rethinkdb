#ifndef __MOCK_DUMMY_PROTOCOL_JSON_ADAPTER_TCC__
#define __MOCK_DUMMY_PROTOCOL_JSON_ADAPTER_TCC__

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
    apply_json_to(change, &target->keys, ctx);
}

template <class ctx_t>
void  on_subfield_change(dummy_protocol_t::region_t *, const ctx_t &) { }
}//namespace mock 

#endif
