#ifndef PROTOB_JSON_SHIM_HPP_
#define PROTOB_JSON_SHIM_HPP_

#include <string>

#include "utils.hpp"

class Query;
class Response;
template<class T>
class scoped_array_t;

namespace json_shim {
MUST_USE bool parse_json_pb(Query *q, char *str) THROWS_NOTHING;
MUST_USE int64_t write_json_pb(const Response *r, std::string *out) THROWS_NOTHING;
} // namespace json_shim;

#endif // PROTOB_JSON_SHIM_HPP_
