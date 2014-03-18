#ifndef PROTOB_JSON_SHIM_HPP_
#define PROTOB_JSON_SHIM_HPP_

#include "utils.hpp"

class Query;

namespace json_shim {
bool parse_json_pb(Query *q, char *str);
} // namespace json_shim;

#endif // PROTOB_JSON_SHIM_HEPP_
