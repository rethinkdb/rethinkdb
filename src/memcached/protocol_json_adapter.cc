#include "memcached/protocol_json_adapter.hpp"

#include <string>

#include "btree/keys.hpp"
#include "hash_region.hpp"
#include "http/http.hpp"
#include "http/json.hpp"

// json adapter concept for `store_key_t`
json_adapter_if_t::json_adapter_map_t get_json_subfields(store_key_t *) {
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(store_key_t *target) {
    return cJSON_CreateString(percent_escaped_string(key_to_unescaped_str(*target)).c_str());
}

void apply_json_to(cJSON *change, store_key_t *target) {
    std::string str(percent_unescaped_string(get_string(change)));
    if (!unescaped_str_to_key(str.c_str(), str.length(), target)) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a store_key_t.\n", get_string(change).c_str()));
    }
}

void on_subfield_change(store_key_t *) { }



// json adapter for key_range_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(key_range_t *) {
    return json_adapter_if_t::json_adapter_map_t();
}

std::string render_region_as_string(key_range_t *target) {
    scoped_cJSON_t res(cJSON_CreateArray());

    res.AddItemToArray(render_as_json(&target->left));

    if (!target->right.unbounded) {
        res.AddItemToArray(render_as_json(&target->right.key));
    } else {
        res.AddItemToArray(cJSON_CreateNull());
    }

    return res.PrintUnformatted();
}

cJSON *render_as_json(key_range_t *target) {
    return cJSON_CreateString(render_region_as_string(target).c_str());
}

void apply_json_to(cJSON *change, key_range_t *target) {
    // TODO: Can we so casually call get_string on a cJSON object?  What if it's not a string?
    scoped_cJSON_t js(cJSON_Parse(get_string(change).c_str()));
    if (js.get() == NULL) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a key_range_t.", get_string(change).c_str()));
    }

    /* TODO: If something other than an array is passed here, then it will crash
    rather than report the error to the user. */
    json_array_iterator_t it(js.get());

    cJSON *first = it.next();
    if (first == NULL) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a key_range_t.", get_string(change).c_str()));
    }

    cJSON *second = it.next();
    if (second == NULL) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a key_range_t.", get_string(change).c_str()));
    }

    if (it.next() != NULL) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a key_range_t.", get_string(change).c_str()));
    }

    try {
        store_key_t left;
        apply_json_to(first, &left);
        if (second->type == cJSON_NULL) {
            *target = key_range_t(key_range_t::closed, left,
                                  key_range_t::none,   store_key_t(""));
        } else {
            store_key_t right;
            apply_json_to(second, &right);
            *target = key_range_t(key_range_t::closed, left,
                                  key_range_t::open,   right);
        }
    } catch (std::runtime_error) {  // TODO Explain wtf can throw a std::runtime_error to us here.
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a memcached_protocol_t::region_t.\n", get_string(change).c_str()));
    }
}

void on_subfield_change(key_range_t *) { }




// json adapter for hash_region_t<key_range_t>

// TODO: This is extremely ghetto: we assert that the hash region isn't split by hash value (because why should the UI ever be exposed to that?) and then only serialize the key range.
json_adapter_if_t::json_adapter_map_t get_json_subfields(UNUSED hash_region_t<key_range_t> *target) {
    return json_adapter_if_t::json_adapter_map_t();
}

std::string render_region_as_string(hash_region_t<key_range_t> *target) {
    // TODO: ghetto low level hash_region_t assertion.
    // TODO: Reintroduce this ghetto low level hash_region_t assertion.
    // guarantee(target->beg == 0 && target->end == HASH_REGION_HASH_SIZE);

    return render_region_as_string(&target->inner);
}

cJSON *render_as_json(hash_region_t<key_range_t> *target) {
    return cJSON_CreateString(render_region_as_string(target).c_str());
}

void apply_json_to(cJSON *change, hash_region_t<key_range_t> *target) {
    target->beg = 0;
    target->end = HASH_REGION_HASH_SIZE;
    apply_json_to(change, &target->inner);
}

void on_subfield_change(hash_region_t<key_range_t> *) { }

