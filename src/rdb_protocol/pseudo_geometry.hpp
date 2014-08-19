// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PSEUDO_GEOMETRY_HPP_
#define RDB_PROTOCOL_PSEUDO_GEOMETRY_HPP_

#include "containers/counted.hpp"

namespace ql {
class datum_t;
class configured_limits_t;

namespace pseudo {

extern const char *const geometry_string;

datum_t geo_sub(datum_t lhs,
                                 datum_t rhs,
                                 const configured_limits_t &limits);

void sanitize_geometry(datum_t *geo);

} // namespace pseudo
} // namespace ql

#endif  // RDB_PROTOCOL_PSEUDO_GEOMETRY_HPP_
