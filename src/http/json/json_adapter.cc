#include "http/json/json_adapter.hpp"

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
        BREAKPOINT;
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
