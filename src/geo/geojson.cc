// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "geo/geojson.hpp"

#include <string>

#include "containers/scoped.hpp"
#include "containers/wire_string.hpp"
#include "geo/geo_visitor.hpp"
#include "geo/s2/s2.h"
#include "geo/s2/s2polygon.h"
#include "geo/s2/s2polyline.h"
#include "rdb_protocol/datum.hpp"

using ql::datum_t;

scoped_ptr_t<S2Point> coordinates_to_s2point(UNUSED const counted_t<const datum_t> &coords) {
    // TODO!
    return scoped_ptr_t<S2Point>(new S2Point());
}

scoped_ptr_t<S2Polyline> coordinates_to_s2polyline(UNUSED const counted_t<const datum_t> &coords) {
    // TODO!
    return scoped_ptr_t<S2Polyline>(new S2Polyline());
}

scoped_ptr_t<S2Polygon> coordinates_to_s2polygon(UNUSED const counted_t<const datum_t> &coords) {
    // TODO!
    return scoped_ptr_t<S2Polygon>(new S2Polygon());
}

// TODO! What does it throw?
void visit_geojson(geo_visitor_t *visitor, const counted_t<const datum_t> &geojson) {
    const std::string type = geojson->get("type")->as_str().to_std();
    counted_t<const datum_t> coordinates = geojson->get("coordinates");

    if (type == "Point") {
        scoped_ptr_t<S2Point> pt = coordinates_to_s2point(coordinates);
        rassert(pt.has());
        visitor->on_point(*pt);
    } else if (type == "LineString") {
        scoped_ptr_t<S2Polyline> l = coordinates_to_s2polyline(coordinates);
        rassert(l.has());
        visitor->on_line(*l);
    } else if (type == "Polygon") {
        scoped_ptr_t<S2Polygon> poly = coordinates_to_s2polygon(coordinates);
        rassert(poly.has());
        visitor->on_polygon(*poly);
    } else {
        bool valid_geojson =
            type == "MultiPoint"
            || type == "MultiLineString"
            || type == "MultiPolygon"
            || type == "GeometryCollection"
            || type == "Feature"
            || type == "FeatureCollection";
        if (valid_geojson) {
            // TODO! Throw
            crash("GeoJSON type %s not supported", type.c_str());
        } else {
            // TODO! Throw
            crash("Unrecognized GeoJSON type %s", type.c_str());
        }
    }
}
