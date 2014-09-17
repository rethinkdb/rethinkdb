// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PSEUDO_GEOMETRY_HPP_
#define RDB_PROTOCOL_PSEUDO_GEOMETRY_HPP_

#include "rdb_protocol/geo/lat_lon_types.hpp"

namespace ql {
class datum_t;

namespace pseudo {

extern const char *const geometry_string;
extern const char *const geo_latitude_key;
extern const char *const geo_longitude_key;

void sanitize_geometry(datum_t *geo);

datum_t make_geo_coordinate(const lat_lon_point_t &coordinates);

} // namespace pseudo
} // namespace ql

#endif  // RDB_PROTOCOL_PSEUDO_GEOMETRY_HPP_
