// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_GEO_VISITOR_HPP_
#define RDB_PROTOCOL_GEO_GEO_VISITOR_HPP_

#include "rdb_protocol/geo/s2/util/math/vector3.h"

namespace geo {
typedef Vector3_d S2Point;
class S2Polyline;
class S2Polygon;
class S2LatLngRect;
}

template <class return_t>
class s2_geo_visitor_t {
public:
    virtual ~s2_geo_visitor_t() { }

    virtual return_t on_point(const geo::S2Point &) = 0;
    virtual return_t on_line(const geo::S2Polyline &) = 0;
    virtual return_t on_polygon(const geo::S2Polygon &) = 0;
    virtual return_t on_latlngrect(const geo::S2LatLngRect &) = 0;
};

#endif  // RDB_PROTOCOL_GEO_GEO_VISITOR_HPP_
