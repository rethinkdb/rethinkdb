#ifndef PROTOB_JSON_SHIM_HPP_
#define PROTOB_JSON_SHIM_HPP_

#include <string>

#include "rapidjson/stringbuffer.h"
#include "utils.hpp"

class Query;
class Response;
template<class T>
class scoped_array_t;

namespace json_shim {
// `str` must be a null-terminated C-string. It might get modified in unspecified ways.
MUST_USE bool parse_json_pb(Query *q, int64_t token, char *str) THROWS_NOTHING;
// `write_json_pb()` appends the encoded response onto out, leaving any existing
// data intact.
void write_json_pb(const Response &r, rapidjson::StringBuffer *out) THROWS_NOTHING;
}  // namespace json_shim

#endif // PROTOB_JSON_SHIM_HPP_
