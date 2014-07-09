// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef GEO_GEOJSON_HPP_
#define GEO_GEOJSON_HPP_

#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "geo/s2/util/math/vector3.h"

class geo_visitor_t;
namespace ql {
class datum_t;
}
typedef Vector3_d S2Point;
class S2Polyline;
class S2Polygon;

scoped_ptr_t<S2Point> to_s2point(const counted_t<const ql::datum_t> &geojson);
scoped_ptr_t<S2Polyline> to_s2polyline(const counted_t<const ql::datum_t> &geojson);
scoped_ptr_t<S2Polygon> to_s2polygon(const counted_t<const ql::datum_t> &geojson);

void visit_geojson(geo_visitor_t *visitor, const counted_t<const ql::datum_t> &geojson);

void validate_geojson(const counted_t<const ql::datum_t> &geojson);

#endif  // GEO_GEO_VISITOR_HPP_
