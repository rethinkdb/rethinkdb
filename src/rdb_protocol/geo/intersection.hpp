// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_INTERSECTION_HPP_
#define RDB_PROTOCOL_GEO_INTERSECTION_HPP_

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

/* A variant that works on two GeoJSON objects */
bool geo_does_intersect(const ql::datum_t &g1,
                        const ql::datum_t &g2);

/* Variants for each pair of S2 geometry */
bool geo_does_intersect(const geo::S2Point &point,
                        const geo::S2Point &other_point);
bool geo_does_intersect(const geo::S2Polyline &line,
                        const geo::S2Point &other_point);
bool geo_does_intersect(const geo::S2Polygon &polygon,
                        const geo::S2Point &other_point);
bool geo_does_intersect(const geo::S2Point &point,
                        const geo::S2Polyline &other_line);
bool geo_does_intersect(const geo::S2Polyline &line,
                        const geo::S2Polyline &other_line);
bool geo_does_intersect(const geo::S2Polygon &polygon,
                        const geo::S2Polyline &other_line);
bool geo_does_intersect(const geo::S2Point &point,
                        const geo::S2Polygon &other_polygon);
bool geo_does_intersect(const geo::S2Polyline &line,
                        const geo::S2Polygon &other_polygon);
bool geo_does_intersect(const geo::S2Polygon &polygon,
                        const geo::S2Polygon &other_polygon);

#endif  // RDB_PROTOCOL_GEO_INTERSECTION_HPP_
