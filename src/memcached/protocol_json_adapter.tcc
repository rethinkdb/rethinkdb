#ifndef MEMCACHED_PROTOCOL_JSON_ADAPTER_TCC_
#define MEMCACHED_PROTOCOL_JSON_ADAPTER_TCC_

#include "memcached/protocol_json_adapter.hpp"

#include <exception>

#include "http/http.hpp"
#include "http/json.hpp"

//json adapter concept for `store_key_t`
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(store_key_t *, const ctx_t &) {
    return typename json_adapter_if_t<ctx_t>::json_adapter_map_t();
}

template<class ctx_t>
cJSON *render_as_json(store_key_t *target, const ctx_t &) {
    return cJSON_CreateString(percent_escaped_string(key_to_unescaped_str(*target)).c_str());
}

template <class ctx_t>
void apply_json_to(cJSON *change, store_key_t *target, const ctx_t &) {
    if (!unescaped_str_to_key(percent_unescaped_string(get_string(change)).c_str(), target)) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a store_key_t.\n", get_string(change).c_str()));
    }
}

template <class ctx_t>
void  on_subfield_change(store_key_t *, const ctx_t &) { }

//json adapter concept for memcached_protocol_t::region_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(memcached_protocol_t::region_t *, const ctx_t &) {
    return typename json_adapter_if_t<ctx_t>::json_adapter_map_t();
}

template <class ctx_t>
cJSON *render_as_json(memcached_protocol_t::region_t *target, const ctx_t &c) {
    scoped_cJSON_t res(cJSON_CreateArray());

    cJSON_AddItemToArray(res.get(), render_as_json(&target->left, c));

    if (!target->right.unbounded) {
        cJSON_AddItemToArray(res.get(), render_as_json(&target->right.key, c));
    } else {
        cJSON_AddItemToArray(res.get(), cJSON_CreateNull());
    }

    std::string val = cJSON_print_std_string(res.get());
    return cJSON_CreateString(val.c_str());
}

template <class ctx_t>
void apply_json_to(cJSON *change, memcached_protocol_t::region_t *target, const ctx_t &c) {
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
        store_key_t left;
        apply_json_to(first, &left, c);
        if (second->type == cJSON_NULL) {
            *target = memcached_protocol_t::region_t(memcached_protocol_t::region_t::closed, left,
                                                     memcached_protocol_t::region_t::none,   store_key_t(""));
        } else {
            store_key_t right;
            apply_json_to(second, &right, c);
            *target = memcached_protocol_t::region_t(memcached_protocol_t::region_t::closed, left,
                                                     memcached_protocol_t::region_t::open,   right);
        }
    } catch (std::runtime_error) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a memcached_protocol_t::region_t.\n", get_string(change).c_str()));
    }
}

template <class ctx_t>
void  on_subfield_change(memcached_protocol_t::region_t *, const ctx_t &) { }

#endif
