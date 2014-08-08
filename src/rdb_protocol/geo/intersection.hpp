// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_INTERSECTION_HPP_
#define RDB_PROTOCOL_GEO_INTERSECTION_HPP_

#include "containers/counted.hpp"
#include "rdb_protocol/geo/s2/util/math/vector3.h"

typedef Vector3_d S2Point;
class S2Polyline;
class S2Polygon;
namespace ql {
class datum_t;
}

/* A variant that works on two GeoJSON objects */
bool geo_does_intersect(const counted_t<const ql::datum_t> &g1,
                        const counted_t<const ql::datum_t> &g2);

/* Variants for each pair of S2 geometry */
bool geo_does_intersect(const S2Point &point,
                        const S2Point &other_point);
bool geo_does_intersect(const S2Polyline &line,
                        const S2Point &other_point);
bool geo_does_intersect(const S2Polygon &polygon,
                        const S2Point &other_point);
bool geo_does_intersect(const S2Point &point,
                        const S2Polyline &other_line);
bool geo_does_intersect(const S2Polyline &line,
                        const S2Polyline &other_line);
bool geo_does_intersect(const S2Polygon &polygon,
                        const S2Polyline &other_line);
bool geo_does_intersect(const S2Point &point,
                        const S2Polygon &other_polygon);
bool geo_does_intersect(const S2Polyline &line,
                        const S2Polygon &other_polygon);
bool geo_does_intersect(const S2Polygon &polygon,
                        const S2Polygon &other_polygon);

#endif  // RDB_PROTOCOL_GEO_INTERSECTION_HPP_
