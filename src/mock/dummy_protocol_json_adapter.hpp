#ifndef MOCK_DUMMY_PROTOCOL_JSON_ADAPTER_HPP_
#define MOCK_DUMMY_PROTOCOL_JSON_ADAPTER_HPP_

#include "http/json/json_adapter.hpp"
#include "mock/dummy_protocol.hpp"

namespace mock {
//json adapter concept for dummy_protocol_t::region_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(dummy_protocol_t::region_t *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(dummy_protocol_t::region_t *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, dummy_protocol_t::region_t *, const ctx_t &);

template <class ctx_t>
void  on_subfield_change(dummy_protocol_t::region_t *, const ctx_t &);

template <class ctx_t>
std::string render_region_as_string(dummy_protocol_t::region_t *target, const ctx_t &ctx);

}//namespace mock

#include "mock/dummy_protocol_json_adapter.tcc"

#endif /* MOCK_DUMMY_PROTOCOL_JSON_ADAPTER_HPP_ */
