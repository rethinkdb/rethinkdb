#ifndef RDB_PROTOCOL_DATUM_JSON_HPP_
#define RDB_PROTOCOL_DATUM_JSON_HPP_

#include "rapidjson/document.h"
#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/datum.hpp"

namespace ql {
datum_t to_datum(
    const rapidjson::Value &json,
    const configured_limits_t &,
    reql_version_t);
}

#endif  // RDB_PROTOCOL_DATUM_JSON_HPP_
