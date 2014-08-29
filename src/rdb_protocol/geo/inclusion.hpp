// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_INCLUSION_HPP_
#define RDB_PROTOCOL_GEO_INCLUSION_HPP_

#include "containers/counted.hpp"
#include "rdb_protocol/geo/s2/util/math/vector3.h"

namespace geo {
typedef Vector3_d S2Point;
class S2Polyline;
class S2Polygon;
}

namespace ql {
class datum_t;
}

/* A variant that works on a GeoJSON object */
bool geo_does_include(const geo::S2Polygon &polygon,
                      const ql::datum_t &g);

/* Variants for S2 geometry */
bool geo_does_include(const geo::S2Polygon &polygon,
                      const geo::S2Point &g);
bool geo_does_include(const geo::S2Polygon &polygon,
                      const geo::S2Polyline &g);
bool geo_does_include(const geo::S2Polygon &polygon,
                      const geo::S2Polygon &g);

#endif  // RDB_PROTOCOL_GEO_INCLUSION_HPP_
