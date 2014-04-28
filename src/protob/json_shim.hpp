#ifndef PROTOB_JSON_SHIM_HPP_
#define PROTOB_JSON_SHIM_HPP_

#include <string>

#include "utils.hpp"

class Query;
class Response;
template<class T>
class scoped_array_t;

namespace json_shim {
MUST_USE bool parse_json_pb(Query *q, int64_t token, const char *str) THROWS_NOTHING;
void write_json_pb(const Response &r, std::string *out) THROWS_NOTHING;
} // namespace json_shim;

#endif // PROTOB_JSON_SHIM_HPP_
