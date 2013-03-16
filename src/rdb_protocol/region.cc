// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rdb_protocol/region.hpp"

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
    std::string str;
    if (!percent_unescape_string(get_string(change), &str)) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a store_key_t.\n", get_string(change).c_str()));
    }

    if (!unescaped_str_to_key(str.c_str(), str.length(), target)) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a store_key_t.\n", get_string(change).c_str()));
    }
}

void on_subfield_change(store_key_t *) { }

std::string to_string_for_json_key(const store_key_t *target) {
    return percent_escaped_string(key_to_unescaped_str(*target));
}

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
    const std::string change_str = get_string(change);
    scoped_cJSON_t js(cJSON_Parse(change_str.c_str()));
    if (js.get() == NULL) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a key_range_t.", change_str.c_str()));
    }

    /* TODO: If something other than an array is passed here, then it will crash
    rather than report the error to the user. */
    json_array_iterator_t it(js.get());

    cJSON *first = it.next();
    if (first == NULL) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a key_range_t.", change_str.c_str()));
    }

    cJSON *second = it.next();
    if (second == NULL) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a key_range_t.", change_str.c_str()));
    }

    if (it.next() != NULL) {
        throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a key_range_t.", change_str.c_str()));
    }

    store_key_t left;
    apply_json_to(first, &left);
    if (second->type == cJSON_NULL) {
        *target = key_range_t(key_range_t::closed, left,
                              key_range_t::none, store_key_t());
    } else {
        store_key_t right;
        apply_json_to(second, &right);

        if (left > right) {
            throw schema_mismatch_exc_t(strprintf("Failed to parse %s as a key_range_t -- bounds are out of order", change_str.c_str()));
        }

        *target = key_range_t(key_range_t::closed, left,
                              key_range_t::open,   right);
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

std::string to_string_for_json_key(hash_region_t<key_range_t> *target) {
    return render_region_as_string(target);
}

cJSON *render_as_json(hash_region_t<key_range_t> *target) {
    return cJSON_CreateString(render_region_as_string(target).c_str());
}

void apply_json_to(cJSON *change, hash_region_t<key_range_t> *target) {
    apply_json_to(change, &target->inner);
    target->beg = 0;
    target->end = target->inner.is_empty() ? 0 : HASH_REGION_HASH_SIZE;
}

void on_subfield_change(hash_region_t<key_range_t> *) { }





bool compare_range_by_left(const key_range_t &r1, const key_range_t &r2) {
    return r1.left < r2.left;
}

region_join_result_t region_join(const std::vector<key_range_t> &vec, key_range_t *out) THROWS_NOTHING {
    if (vec.empty()) {
        *out = key_range_t::empty();
        return REGION_JOIN_OK;
    } else {
        std::vector<key_range_t> sorted = vec;
        std::sort(sorted.begin(), sorted.end(), &compare_range_by_left);
        key_range_t::right_bound_t cursor = key_range_t::right_bound_t(sorted[0].left);
        for (size_t i = 0; i < sorted.size(); ++i) {
            if (cursor < key_range_t::right_bound_t(sorted[i].left)) {
                return REGION_JOIN_BAD_REGION;
            } else if (cursor > key_range_t::right_bound_t(sorted[i].left)) {
                return REGION_JOIN_BAD_JOIN;
            } else {
                /* The regions match exactly; move on to the next one. */
                cursor = sorted[i].right;
            }
        }
        key_range_t key_union;
        key_union.left = sorted[0].left;
        key_union.right = cursor;
        *out = key_union;
        return REGION_JOIN_OK;
    }
}

std::vector<key_range_t> region_subtract_many(key_range_t minuend, const std::vector<key_range_t>& subtrahends) {
    std::vector<key_range_t> buf, temp_result_buf;
    buf.push_back(minuend);
    for (std::vector<key_range_t>::const_iterator s = subtrahends.begin(); s != subtrahends.end(); ++s) {
        for (std::vector<key_range_t>::const_iterator m = buf.begin(); m != buf.end(); ++m) {
            // We are computing m-s here for each m in buf and s in subtrahends:
            // m-s = m & not(s) = m & ([-inf, s.left) | [s.right, +inf))
            //                  = (m & [-inf, s.left)) | (m & [s.right, +inf))
            key_range_t left = region_intersection(*m, key_range_t(key_range_t::none, store_key_t(), key_range_t::open, (*s).left));
            if (!left.is_empty()) {
                temp_result_buf.push_back(left);
            }
            if (!s->right.unbounded) {
                key_range_t right = region_intersection(*m, key_range_t(key_range_t::closed, s->right.key, key_range_t::none, store_key_t()));

                if (!right.is_empty()) {
                    temp_result_buf.push_back(right);
                }
            }
        }
        buf.swap(temp_result_buf);
        temp_result_buf.clear();
    }
    return buf;
}
