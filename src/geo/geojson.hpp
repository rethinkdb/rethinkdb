// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef GEO_GEOJSON_HPP_
#define GEO_GEOJSON_HPP_

#include <utility>
#include <vector>

#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "geo/lat_lon_types.hpp"
#include "geo/s2/util/math/vector3.h"

class s2_geo_visitor_t;
namespace ql {
class datum_t;
}
typedef Vector3_d S2Point;
class S2Polyline;
class S2Polygon;


/* These functions construct a GeoJSON object of the respective type.
They also insert the correct $reql_type$ field into the output.
They do not perform any validation. */
counted_t<const ql::datum_t> construct_geo_point(const lat_lon_point_t &point);
counted_t<const ql::datum_t> construct_geo_line(const lat_lon_line_t &line);
// Only an outer shell is supported through this. No holes.
// Closes the shell implicitly (i.e. connects the first point to the last point).
counted_t<const ql::datum_t> construct_geo_polygon(const lat_lon_line_t &shell);

/* These functions extract coordinates from GeoJSON objects */
lat_lon_point_t extract_lat_lon_point(const counted_t<const ql::datum_t> &geojson);
lat_lon_line_t extract_lat_lon_line(const counted_t<const ql::datum_t> &geojson);
lat_lon_line_t extract_lat_lon_shell(const counted_t<const ql::datum_t> &geojson);

/* These functions convert from a GeoJSON object to S2 types */
scoped_ptr_t<S2Point> to_s2point(const counted_t<const ql::datum_t> &geojson);
scoped_ptr_t<S2Polyline> to_s2polyline(const counted_t<const ql::datum_t> &geojson);
scoped_ptr_t<S2Polygon> to_s2polygon(const counted_t<const ql::datum_t> &geojson);

/* Performs conversion to an S2 type and calls the visitor, depending on the geometry
type in the GeoJSON object. */
void visit_geojson(s2_geo_visitor_t *visitor, const counted_t<const ql::datum_t> &geojson);

/* Checks the semantic and syntactic validity of a GeoJSON object. */
void validate_geojson(const counted_t<const ql::datum_t> &geojson);

#endif  // GEO_GEOJSON_HPP_
