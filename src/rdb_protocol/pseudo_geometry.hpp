// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PSEUDO_GEOMETRY_HPP_
#define RDB_PROTOCOL_PSEUDO_GEOMETRY_HPP_

#include "containers/counted.hpp"

namespace ql {
class datum_t;

namespace pseudo {

extern const char *const geometry_string;

counted_t<const datum_t> geo_sub(counted_t<const datum_t> lhs,
                                 counted_t<const datum_t> rhs);

} // namespace pseudo
} // namespace ql

#endif  // RDB_PROTOCOL_PSEUDO_GEOMETRY_HPP_
