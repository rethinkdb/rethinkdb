// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PSEUDO_GEOMETRY_HPP_
#define RDB_PROTOCOL_PSEUDO_GEOMETRY_HPP_

namespace ql {
class datum_t;

namespace pseudo {

extern const char *const geometry_string;

void sanitize_geometry(datum_t *geo);

} // namespace pseudo
} // namespace ql

#endif  // RDB_PROTOCOL_PSEUDO_GEOMETRY_HPP_
