#ifndef __MEMCACHED_PROTOCOL_JSON_ADAPTER_TCC__
#define __MEMCACHED_PROTOCOL_JSON_ADAPTER_TCC__

#include <exception>

#include "http/http.hpp"
#include "http/json.hpp"

//json adapter concept for memcached_protocol_t::region_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(memcached_protocol_t::region_t *, const ctx_t &) {
    return typename json_adapter_if_t<ctx_t>::json_adapter_map_t();
}

template <class ctx_t>
cJSON *render_as_json(memcached_protocol_t::region_t *target, const ctx_t &) {
    scoped_cJSON_t res(cJSON_CreateArray());

    cJSON_AddItemToArray(res.get(), cJSON_CreateString(percent_escaped_string(key_to_str(target->left)).c_str()));

    if (!target->right.unbounded) {
        cJSON_AddItemToArray(res.get(), cJSON_CreateString(percent_escaped_string(key_to_str(target->right.key)).c_str()));
    } else {
        cJSON_AddItemToArray(res.get(), cJSON_CreateNull());
    }

    std::string val = cJSON_print_std_string(res.get());
    return cJSON_CreateString(val.c_str());
}

template <class ctx_t>
void apply_json_to(cJSON *change, memcached_protocol_t::region_t *target, const ctx_t &) {
    scoped_cJSON_t js(cJSON_Parse(get_string(change).c_str()));
    if (js.get() == NULL) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a memcached_protocol_t::region_t.\n", get_string(change).c_str()));
    }

    /* TODO: If something other than an array is passed here, then it will crash
    rather than report the error to the user. */
    json_array_iterator_t it(js.get());

    cJSON *first = it.next();
    if (first == NULL) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a memcached_protocol_t::region_t.\n", get_string(change).c_str()));
    }

    cJSON *second = it.next();
    if (second == NULL) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a memcached_protocol_t::region_t.\n", get_string(change).c_str()));
    }

    if (it.next() != NULL) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a memcached_protocol_t::region_t.\n", get_string(change).c_str()));
    }

    try {
        if (second->type == cJSON_NULL) {
            *target = memcached_protocol_t::region_t(memcached_protocol_t::region_t::closed, store_key_t(percent_unescaped_string(get_string(first))), 
                                                     memcached_protocol_t::region_t::none,   store_key_t(""));
        } else {
            *target = memcached_protocol_t::region_t(memcached_protocol_t::region_t::closed, store_key_t(percent_unescaped_string(get_string(first))), 
                                                     memcached_protocol_t::region_t::open,   store_key_t(percent_unescaped_string(get_string(second))));
        }
    } catch (std::runtime_error) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a memcached_protocol_t::region_t.\n", get_string(change).c_str()));
    }
}

template <class ctx_t>
void  on_subfield_change(memcached_protocol_t::region_t *, const ctx_t &) { }

#endif
