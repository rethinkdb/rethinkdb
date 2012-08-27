#include "http/json/json_adapter.hpp"

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
        throw schema_mismatch_exc_t(strprintf("Expected bool instead got: %s\n", cJSON_print_std_string(json).c_str()).c_str());
    }
}

std::string get_string(cJSON *json) {
    if (json->type == cJSON_String) {
        return std::string(json->valuestring);
    } else {
        throw schema_mismatch_exc_t(strprintf("Expected string instead got: %s\n", cJSON_print_std_string(json).c_str()).c_str());
    }
}

int get_int(cJSON *json) {
    if (json->type == cJSON_Number) {
        return json->valueint;
    } else {
        throw schema_mismatch_exc_t(strprintf("Expected int instead got: %s\n", cJSON_print_std_string(json).c_str()).c_str());
    }
}

double get_double(cJSON *json) {
    if (json->type == cJSON_Number) {
        return json->valuedouble;
    } else {
        throw schema_mismatch_exc_t(strprintf("Expected double instead got: %s\n", cJSON_print_std_string(json).c_str()).c_str());
    }
}

json_array_iterator_t get_array_it(cJSON *json) {
    if (json->type == cJSON_Array) {
        return json_array_iterator_t(json);
    } else {
        //BREAKPOINT;
        throw schema_mismatch_exc_t(strprintf("Expected array instead got: %s\n", cJSON_print_std_string(json).c_str()).c_str());
    }
}

json_object_iterator_t get_object_it(cJSON *json) {
    if (json->type == cJSON_Object) {
        return json_object_iterator_t(json);
    } else {
        throw schema_mismatch_exc_t(strprintf("Expected object instead got: %s\n", cJSON_print_std_string(json).c_str()).c_str());
    }
}


//implementation for json_adapter_if_t
json_adapter_if_t::json_adapter_map_t json_adapter_if_t::get_subfields() {
    json_adapter_map_t res = get_subfields_impl();
    for (json_adapter_map_t::iterator it = res.begin(); it != res.end(); it++) {
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
    } catch (std::runtime_error e) {
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

// ctx-less JSON adapter for time_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(time_t *) {
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(time_t *target) {
    return cJSON_CreateNumber(*target);
}

void apply_json_to(cJSON *change, time_t *target) {
    *target = get_int(change);
}

void on_subfield_change(time_t *) { }


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

void on_subfield_change(bool *) { }

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

void on_subfield_change(std::string *) { }

}  // namespace std


// ctx-less JSON adapter for uuid_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(uuid_t *) {
    return std::map<std::string, boost::shared_ptr<json_adapter_if_t> >();
}

cJSON *render_as_json(const uuid_t *uuid) {
    if (uuid->is_nil()) {
        return cJSON_CreateNull();
    } else {
        return cJSON_CreateString(uuid_to_str(*uuid).c_str());
    }
}

void apply_json_to(cJSON *change, uuid_t *uuid) {
    if (change->type == cJSON_NULL) {
        *uuid = nil_uuid();
    } else {
        try {
            *uuid = str_to_uuid(get_string(change));
        } catch (std::runtime_error) {
            throw schema_mismatch_exc_t(strprintf("String %s, did not parse as uuid", cJSON_print_unformatted_std_string(change).c_str()));
        }
    }
}

void on_subfield_change(uuid_t *) { }



// ctx-less JSON adapter for uint64_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(uint64_t *) {
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(uint64_t *target) {
    return cJSON_CreateNumber(*target);
}

void apply_json_to(cJSON *change, uint64_t *target) {
    *target = get_int(change);
}

void on_subfield_change(uint64_t *) { }


// ctx-less JSON adapter for int
json_adapter_if_t::json_adapter_map_t get_json_subfields(int *) {
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(int *target) {
    return cJSON_CreateNumber(*target);
}

void apply_json_to(cJSON *change, int *target) {
    *target = get_int(change);
}

void on_subfield_change(int *) { }
