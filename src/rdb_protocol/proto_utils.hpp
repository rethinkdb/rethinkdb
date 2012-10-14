#ifndef __RDB_PROTO_UTILS_HPP__
#define __RDB_PROTO_UTILS_HPP__

#include <string>
#include "rdb_protocol/backtrace.hpp"
#include "rdb_protocol/exceptions.hpp"
#include "http/json.hpp"
#include "utils.hpp"

using namespace query_language;

std::string cJSON_print_primary(cJSON *json, const backtrace_t &backtrace);

#ifndef NDEBUG
#define guarantee_debug_throw_release(cond, backtrace) guarantee(cond)
#else
#define guarantee_debug_throw_release(cond, backtrace) do {                                     \
    if (!(cond)) {                                                                              \
        throw runtime_exc_t(format_assert_message("BROKEN SERVER GUARANTEE", cond), backtrace); \
    }                                                                                           \
} while (0)
#endif // NDEBUG



#endif // __RDB_PROTO_UTILS_HPP__

