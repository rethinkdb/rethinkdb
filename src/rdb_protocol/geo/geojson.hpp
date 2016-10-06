// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_GEOJSON_HPP_
#define RDB_PROTOCOL_GEO_GEOJSON_HPP_

#include <string>
#include <utility>
#include <vector>

#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/geo/geo_visitor.hpp"
#include "rdb_protocol/geo/lon_lat_types.hpp"
#include "rdb_protocol/geo/s2/util/math/vector3.h"
#include "rdb_protocol/datum.hpp"

namespace geo {
class S2LatLngRect;
typedef Vector3_d S2Point;
class S2Polyline;
class S2Polygon;
}

namespace ql {
class configured_limits_t;
}


/* These functions construct a GeoJSON object of the respective type.
They also insert the correct $reql_type$ field into the output.
They do not perform any validation. */
ql::datum_t construct_geo_point(
        const lon_lat_point_t &point,
        const ql::configured_limits_t &limits);
ql::datum_t construct_geo_line(
        const lon_lat_line_t &line,
        const ql::configured_limits_t &limits);
// Closes the shell implicitly (i.e. connects the first point to the last point).
ql::datum_t construct_geo_polygon(
        const lon_lat_line_t &shell,
        const ql::configured_limits_t &limits);
ql::datum_t construct_geo_polygon(
        const lon_lat_line_t &shell,
        const std::vector<lon_lat_line_t> &holes,
        const ql::configured_limits_t &limits);
// This is not part of the GeoJSON standard, but our own extension that we use
// internally for `getIntersecting` changefeeds.
ql::datum_t construct_geo_latlngrect(
        const geo::S2LatLngRect &rect,
        const ql::configured_limits_t &limits);

/* These functions extract coordinates from GeoJSON objects */
lon_lat_point_t extract_lon_lat_point(const ql::datum_t &geojson);
lon_lat_line_t extract_lon_lat_line(const ql::datum_t &geojson);
lon_lat_line_t extract_lon_lat_shell(const ql::datum_t &geojson);

/* These functions convert from a GeoJSON object to S2 types */
scoped_ptr_t<geo::S2Point> to_s2point(const ql::datum_t &geojson);
scoped_ptr_t<geo::S2Polyline> to_s2polyline(const ql::datum_t &geojson);
scoped_ptr_t<geo::S2Polygon> to_s2polygon(const ql::datum_t &geojson);

/* Helpers for visit_geojson() */
scoped_ptr_t<geo::S2Point> coordinates_to_s2point(
        const ql::datum_t &coords);
scoped_ptr_t<geo::S2Polyline> coordinates_to_s2polyline(
        const ql::datum_t &coords);
scoped_ptr_t<geo::S2Polygon> coordinates_to_s2polygon(
        const ql::datum_t &coords);
scoped_ptr_t<geo::S2LatLngRect> coordinates_to_s2latlngrect(
        const ql::datum_t &coords);

/* Performs conversion to an S2 type and calls the visitor, depending on the geometry
type in the GeoJSON object. */
template <class return_t>
return_t visit_geojson(
        s2_geo_visitor_t<return_t> *visitor,
        const ql::datum_t &geojson) {
    datum_string_t type = geojson.get_field("type").as_str();
    ql::datum_t coordinates = geojson.get_field("coordinates");

    if (type == "Point") {
        scoped_ptr_t<geo::S2Point> pt = coordinates_to_s2point(coordinates);
        rassert(pt.has());
        return visitor->on_point(*pt);
    } else if (type == "LineString") {
        scoped_ptr_t<geo::S2Polyline> l = coordinates_to_s2polyline(coordinates);
        rassert(l.has());
        return visitor->on_line(*l);
    } else if (type == "Polygon") {
        scoped_ptr_t<geo::S2Polygon> poly = coordinates_to_s2polygon(coordinates);
        rassert(poly.has());
        return visitor->on_polygon(*poly);
    } else if (type == "$reql_LatLngRect$") {
        scoped_ptr_t<geo::S2LatLngRect> rect = coordinates_to_s2latlngrect(coordinates);
        rassert(rect.has());
        return visitor->on_latlngrect(*rect);
    } else {
        bool valid_geojson =
            type == "MultiPoint"
            || type == "MultiLineString"
            || type == "MultiPolygon"
            || type == "GeometryCollection"
            || type == "Feature"
            || type == "FeatureCollection";
        if (valid_geojson) {
            throw geo_exception_t(
                strprintf("GeoJSON type `%s` is not supported.", type.to_std().c_str()));
        } else {
            throw geo_exception_t(
                strprintf("Unrecognized GeoJSON type `%s`.", type.to_std().c_str()));
        }
    }
}

/* Checks the semantic and syntactic validity of a GeoJSON object. */
void validate_geojson(const ql::datum_t &geojson);

#endif  // RDB_PROTOCOL_GEO_GEOJSON_HPP_
