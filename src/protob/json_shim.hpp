#ifndef PROTOB_JSON_SHIM_HPP_
#define PROTOB_JSON_SHIM_HPP_

#include "utils.hpp"

class Query;
class Response;
template<class T>
class scoped_array_t;

namespace json_shim {
bool parse_json_pb(Query *q, char *str) throw ();
std::string write_json_pb(const Response *r) throw ();
} // namespace json_shim;

#endif // PROTOB_JSON_SHIM_HEPP_
