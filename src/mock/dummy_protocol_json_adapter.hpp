#ifndef MOCK_DUMMY_PROTOCOL_JSON_ADAPTER_HPP_
#define MOCK_DUMMY_PROTOCOL_JSON_ADAPTER_HPP_

#include <string>

#include "http/json/json_adapter.hpp"
#include "mock/dummy_protocol.hpp"

namespace mock {

// ctx-less json adapter concept for dummy_protocol_t::region_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(dummy_protocol_t::region_t *);

cJSON *render_as_json(dummy_protocol_t::region_t *);

void apply_json_to(cJSON *, dummy_protocol_t::region_t *);

void  on_subfield_change(dummy_protocol_t::region_t *);

std::string render_region_as_string(dummy_protocol_t::region_t *target);


}//namespace mock

#include "mock/dummy_protocol_json_adapter.tcc"

#endif /* MOCK_DUMMY_PROTOCOL_JSON_ADAPTER_HPP_ */
