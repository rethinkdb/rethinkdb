// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "http/json/json_adapter.hpp"

#include <inttypes.h>
#include <limits.h>

#include "utils.hpp"

static const std::string schema_mismatch_msg(cJSON *json, const char *expected) {
    const char * got = cJSON_type_to_string(json->type);
    std::string val = cJSON_print_std_string(json);

    return strprintf("Expected '%s' instead got '%s': %s\n", expected, got, val.c_str());
}

bool is_null(cJSON *json) {
    return json->type == cJSON_NULL;
}

//Functions to make accessing cJSON *objects easier

bool get_bool(cJSON *json) {
    if (json->type == cJSON_True) {
        return true;
    } else if (json->type == cJSON_False) {
        return false;
    } else {
        throw schema_mismatch_exc_t(schema_mismatch_msg(json, "bool").c_str());
    }
}

std::string get_string(cJSON *json) {
    if (json->type == cJSON_String) {
        return std::string(json->valuestring);
    } else {
        throw schema_mismatch_exc_t(schema_mismatch_msg(json, "string").c_str());
    }
}

int64_t get_int(cJSON *json, int64_t min_value, int64_t max_value) {
    if (json->type == cJSON_Number) {
        int64_t value = static_cast<int64_t>(json->valuedouble);
        if (static_cast<double>(value) != json->valuedouble) {
            throw schema_mismatch_exc_t(schema_mismatch_msg(json, "int").c_str());
        }
        if (value < min_value || value > max_value) {
            throw schema_mismatch_exc_t(strprintf("Expected int in range (%" PRIi64 " to %" PRIi64 ") but instead got: %s\n", min_value, max_value, cJSON_print_std_string(json).c_str()).c_str());
        }

        return value;
    } else {
        throw schema_mismatch_exc_t(schema_mismatch_msg(json, "int").c_str());
    }
}

double get_double(cJSON *json) {
    if (json->type == cJSON_Number) {
        return json->valuedouble;
    } else {
        throw schema_mismatch_exc_t(schema_mismatch_msg(json, "double").c_str());
    }
}

json_array_iterator_t get_array_it(cJSON *json) {
    if (json->type == cJSON_Array) {
        return json_array_iterator_t(json);
    } else {
        throw schema_mismatch_exc_t(schema_mismatch_msg(json, "array").c_str());
    }
}

json_object_iterator_t get_object_it(cJSON *json) {
    if (json->type == cJSON_Object) {
        return json_object_iterator_t(json);
    } else {
        throw schema_mismatch_exc_t(schema_mismatch_msg(json, "object").c_str());
    }
}


//implementation for json_adapter_if_t
json_adapter_if_t::json_adapter_map_t json_adapter_if_t::get_subfields() {
    json_adapter_map_t res = get_subfields_impl();
    for (json_adapter_map_t::iterator it = res.begin(); it != res.end(); ++it) {
        it->second->superfields.insert(it->second->superfields.end(),
                                       superfields.begin(),
                                       superfields.end());

        it->second->superfields.push_back(get_change_callback());
    }
    return res;
}

cJSON *json_adapter_if_t::render() {
    return render_impl();
}

void json_adapter_if_t::apply(cJSON *change) {
    try {
        apply_impl(change);
    } catch (const std::runtime_error &e) {
        std::string s = cJSON_print_std_string(change);
        throw schema_mismatch_exc_t(strprintf("Failed to apply change: %s", s.c_str()));
    }
    boost::shared_ptr<subfield_change_functor_t> change_callback = get_change_callback();
    if (change_callback) {
        get_change_callback()->on_change();
    }

    for (std::vector<boost::shared_ptr<subfield_change_functor_t> >::iterator it = superfields.begin();
         it != superfields.end();
         ++it) {
        if (*it) {
            (*it)->on_change();
        }
    }
}

void json_adapter_if_t::erase() {
    erase_impl();

    boost::shared_ptr<subfield_change_functor_t> change_callback = get_change_callback();
    if (change_callback) {
        get_change_callback()->on_change();
    }

    for (std::vector<boost::shared_ptr<subfield_change_functor_t> >::iterator it = superfields.begin();
         it != superfields.end();
         ++it) {
        if (*it) {
            (*it)->on_change();
        }
    }
}

void json_adapter_if_t::reset() {
    reset_impl();

    boost::shared_ptr<subfield_change_functor_t> change_callback = get_change_callback();
    if (change_callback) {
        get_change_callback()->on_change();
    }

    for (std::vector<boost::shared_ptr<subfield_change_functor_t> >::iterator it = superfields.begin();
         it != superfields.end();
         ++it) {
        if (*it) {
            (*it)->on_change();
        }
    }
}

// ctx-less JSON adapter for bool
json_adapter_if_t::json_adapter_map_t get_json_subfields(bool *) {
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(bool *target) {
    return cJSON_CreateBool(*target);
}

void apply_json_to(cJSON *change, bool *target) {
    *target = get_bool(change);
}

namespace std {

// ctx-less JSON adapter for std::string

json_adapter_if_t::json_adapter_map_t get_json_subfields(std::string *) {
    return std::map<std::string, boost::shared_ptr<json_adapter_if_t> >();
}

cJSON *render_as_json(std::string *target) {
    return cJSON_CreateString(target->c_str());
}

void apply_json_to(cJSON *change, std::string *target) {
    *target = get_string(change);
}

std::string to_string_for_json_key(const std::string *s) {
    return *s;
}

}  // namespace std


// ctx-less JSON adapter for uuid_u
json_adapter_if_t::json_adapter_map_t get_json_subfields(uuid_u *) {
    return std::map<std::string, boost::shared_ptr<json_adapter_if_t> >();
}

cJSON *render_as_json(const uuid_u *uuid) {
    return cJSON_CreateString(uuid_to_str(*uuid).c_str());
}

void apply_json_to(cJSON *change, uuid_u *uuid) {
    if (change->type == cJSON_NULL) {
        *uuid = nil_uuid();
    } else {
        try {
            *uuid = str_to_uuid(get_string(change));
        } catch (const std::runtime_error &) {
            throw schema_mismatch_exc_t(strprintf("String %s, did not parse as uuid", cJSON_print_unformatted_std_string(change).c_str()));
        }
    }
}

std::string to_string_for_json_key(const uuid_u *uuid) {
    return uuid_to_str(*uuid);
}

// ctx-less JSON adapter for unsigned long long
json_adapter_if_t::json_adapter_map_t get_json_subfields(unsigned long long *) {  // NOLINT(runtime/int)
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(unsigned long long *target) {  // NOLINT(runtime/int)
    // TODO: Should we not fail when we cannot convert to double (or when we go outside 53-bit range?)
    return cJSON_CreateNumber(*target);
}

void apply_json_to(cJSON *change, unsigned long long *target) {  // NOLINT(runtime/int)
    *target = get_int(change, 0, ULLONG_MAX);
}

// ctx-less JSON adapter for long long.
json_adapter_if_t::json_adapter_map_t get_json_subfields(long long *) {  // NOLINT(runtime/int)
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(long long *target) {  // NOLINT(runtime/int)
    // TODO: Should we not fail when we cannot convert to double (or when we go outside 53-bit range?)
    return cJSON_CreateNumber(*target);
}

void apply_json_to(cJSON *change, long long *target) {  // NOLINT(runtime/int)
    *target = get_int(change, LLONG_MIN, LLONG_MAX);
}

// ctx-less JSON adapter for unsigned long.
json_adapter_if_t::json_adapter_map_t get_json_subfields(unsigned long *) {  // NOLINT(runtime/int)
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(unsigned long *target) {  // NOLINT(runtime/int)
    // TODO: Should we not fail when we cannot convert to double (or when we go outside 53-bit range?)
    return cJSON_CreateNumber(*target);
}

void apply_json_to(cJSON *change, unsigned long *target) {  // NOLINT(runtime/int)
    *target = get_int(change, 0, ULONG_MAX);
}


// ctx-less JSON adapter for long.
json_adapter_if_t::json_adapter_map_t get_json_subfields(long *) {  // NOLINT(runtime/int)
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(long *target) {  // NOLINT(runtime/int)
    // TODO: Should we not fail when we cannot convert to double (or when we go outside 53-bit range?)
    return cJSON_CreateNumber(*target);
}

void apply_json_to(cJSON *change, long *target) {  // NOLINT(runtime/int)
    *target = get_int(change, LONG_MIN, LONG_MAX);
}


// ctx-less JSON adapter for int
json_adapter_if_t::json_adapter_map_t get_json_subfields(int *) {
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(int *target) {
    // TODO: Should we not fail when we cannot convert to double (or when we go outside 53-bit range?)
    return cJSON_CreateNumber(*target);
}

void apply_json_to(cJSON *change, int *target) {
    *target = get_int(change, INT_MIN, INT_MAX);
}



// ctx-less JSON adapter for unsigned int
json_adapter_if_t::json_adapter_map_t get_json_subfields(unsigned int *) {
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(unsigned int *target) {
    // TODO: Should we not fail when we cannot convert to double (or when we go outside 53-bit range?)
    return cJSON_CreateNumber(*target);
}

void apply_json_to(cJSON *change, unsigned int *target) {
    *target = get_int(change, 0, UINT_MAX);
}
