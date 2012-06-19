#ifndef MEMCACHED_PROTOCOL_JSON_ADAPTER_HPP_
#define MEMCACHED_PROTOCOL_JSON_ADAPTER_HPP_

#include "memcached/protocol.hpp"
#include "http/json/json_adapter.hpp"

//json adapter concept for store_key_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(store_key_t *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(store_key_t *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, store_key_t *, const ctx_t &);

template <class ctx_t>
void  on_subfield_change(store_key_t *, const ctx_t &);

//json adapter concept for memcached_protocol_t::region_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(memcached_protocol_t::region_t *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(memcached_protocol_t::region_t *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, memcached_protocol_t::region_t *, const ctx_t &);

template <class ctx_t>
void  on_subfield_change(memcached_protocol_t::region_t *, const ctx_t &);

template <class ctx_t>
std::string render_region_as_string(memcached_protocol_t::region_t *target, const ctx_t &ctx);

#include "memcached/protocol_json_adapter.tcc"

#endif /* MEMCACHED_PROTOCOL_JSON_ADAPTER_HPP_ */
