#include "http/json/json_adapter.hpp"

//Functions to make accessing cJSON *objects easier

bool get_bool(const cJSON *json) {
    if (json->type == cJSON_True) {
        return true;
    } else if (json->type == cJSON_False) {
        return false;
    } else {
        throw schema_mismatch_exc_t("Expected bool\n");
    }
}

std::string get_string(cJSON *json) {
    if (json->type == cJSON_String) {
        return std::string(json->valuestring);
    } else {
        throw schema_mismatch_exc_t("Expected string\n");
    }
}

int get_int(const cJSON *json) {
    if (json->type == cJSON_Number) {
        return json->valueint;
    } else {
        throw schema_mismatch_exc_t("Expected int\n");
    }
}

double get_double(const cJSON *json) {
    if (json->type == cJSON_Number) {
        return json->valuedouble;
    } else {
        throw schema_mismatch_exc_t("Expected double\n");
    }
}
