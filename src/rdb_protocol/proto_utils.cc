
#include "rdb_protocol/proto_utils.hpp"

std::string cJSON_print_primary(cJSON *json, const backtrace_t &backtrace) {
    guarantee_debug_throw_release(json, backtrace);
    if (json->type != cJSON_Number && json->type != cJSON_String) {
        throw runtime_exc_t(strprintf("Primary key must be a number or a string, not %s",
                                      cJSON_print_std_string(json).c_str()), backtrace);
    }
    std::string s = cJSON_print_lexicographic(json);
    if (s.size() > MAX_KEY_SIZE) {
        throw runtime_exc_t(strprintf("Primary key too long (max %d characters): %s",
                                      MAX_KEY_SIZE-1, cJSON_print_std_string(json).c_str()), backtrace);
    }
    return s;
}

