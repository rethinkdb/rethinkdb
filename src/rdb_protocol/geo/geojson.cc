// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/geo/geojson.hpp"

#include <string>
#include <vector>

#include "containers/scoped.hpp"
#include "rdb_protocol/datum_string.hpp"
#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/geo/geo_visitor.hpp"
#include "rdb_protocol/geo/s2/s1angle.h"
#include "rdb_protocol/geo/s2/s2.h"
#include "rdb_protocol/geo/s2/s2latlng.h"
#include "rdb_protocol/geo/s2/s2loop.h"
#include "rdb_protocol/geo/s2/s2polygon.h"
#include "rdb_protocol/geo/s2/s2polygonbuilder.h"
#include "rdb_protocol/geo/s2/s2polyline.h"
#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/pseudo_geometry.hpp"

using geo::S1Angle;
using geo::S2Point;
using geo::S2Polygon;
using geo::S2PolygonBuilder;
using geo::S2PolygonBuilderOptions;
using geo::S2Polyline;
using geo::S2LatLng;
using geo::S2Loop;
using ql::datum_t;
using ql::datum_object_builder_t;
using ql::configured_limits_t;

ql::datum_t construct_geo_point(
        const lon_lat_point_t &point,
        const configured_limits_t &limits) {
    datum_object_builder_t result;
    bool dup;
    dup = result.add(datum_t::reql_type_string,
        datum_t(ql::pseudo::geometry_string));
    r_sanity_check(!dup);
    dup = result.add("type", datum_t("Point"));
    r_sanity_check(!dup);

    std::vector<datum_t> coordinates;
    coordinates.reserve(2);
    coordinates.push_back(datum_t(point.longitude));
    coordinates.push_back(datum_t(point.latitude));
    dup = result.add("coordinates",
        datum_t(std::move(coordinates), limits));
    r_sanity_check(!dup);

    return std::move(result).to_datum();
}

std::vector<datum_t> construct_line_coordinates(
        const lon_lat_line_t &line,
        const configured_limits_t &limits) {
    std::vector<datum_t> coordinates;
    coordinates.reserve(line.size());
    for (size_t i = 0; i < line.size(); ++i) {
        std::vector<datum_t> point;
        point.reserve(2);
        point.push_back(datum_t(line[i].longitude));
        point.push_back(datum_t(line[i].latitude));
        coordinates.push_back(datum_t(std::move(point), limits));
    }
    return coordinates;
}

std::vector<datum_t> construct_loop_coordinates(
        const lon_lat_line_t &line,
        const configured_limits_t &limits) {
    std::vector<datum_t> loop_coordinates =
        construct_line_coordinates(line, limits);
    // Close the line
    if (line.size() >= 1 && line[0] != line[line.size()-1]) {
        rassert(!loop_coordinates.empty());
        loop_coordinates.push_back(loop_coordinates[0]);
    }
    return loop_coordinates;
}

ql::datum_t construct_geo_line(
        const lon_lat_line_t &line,
        const configured_limits_t &limits) {
    datum_object_builder_t result;
    bool dup;
    dup = result.add(datum_t::reql_type_string,
        datum_t(ql::pseudo::geometry_string));
    r_sanity_check(!dup);
    dup = result.add("type", datum_t("LineString"));
    r_sanity_check(!dup);

    dup = result.add("coordinates",
        datum_t(construct_line_coordinates(line, limits), limits));
    r_sanity_check(!dup);

    return std::move(result).to_datum();
}

ql::datum_t construct_geo_polygon(
        const lon_lat_line_t &shell,
        const configured_limits_t &limits) {
    std::vector<lon_lat_line_t> holes;
    return construct_geo_polygon(shell, holes, limits);
}

ql::datum_t construct_geo_polygon(
        const lon_lat_line_t &shell,
        const std::vector<lon_lat_line_t> &holes,
        const configured_limits_t &limits) {
    datum_object_builder_t result;
    bool dup;
    dup = result.add(datum_t::reql_type_string,
        datum_t(ql::pseudo::geometry_string));
    r_sanity_check(!dup);
    dup = result.add("type", datum_t("Polygon"));
    r_sanity_check(!dup);

    std::vector<datum_t> coordinates;
    std::vector<datum_t> shell_coordinates =
        construct_loop_coordinates(shell, limits);
    coordinates.push_back(
        datum_t(std::move(shell_coordinates), limits));
    for (size_t i = 0; i < holes.size(); ++i) {
        std::vector<datum_t> hole_coordinates =
            construct_loop_coordinates(holes[i], limits);
        coordinates.push_back(
            datum_t(std::move(hole_coordinates), limits));
    }
    dup = result.add("coordinates",
        datum_t(std::move(coordinates), limits));
    r_sanity_check(!dup);

    return std::move(result).to_datum();
}

// Parses a GeoJSON "Position" array
lon_lat_point_t position_to_lon_lat_point(const datum_t &position) {
    // This assumes the default spherical GeoJSON coordinate reference system,
    // with latitude and longitude given in degrees.

    const size_t arr_size = position.arr_size();
    if (arr_size < 2) {
        throw geo_exception_t(
            "Too few coordinates.  Need at least longitude and latitude.");
    }
    if (arr_size > 3) {
        throw geo_exception_t(
            strprintf("Too many coordinates.  GeoJSON position should have no more than "
                      "three coordinates, but got %zu.", arr_size));
    }
    if (arr_size == 3) {
        throw geo_exception_t("A third altitude coordinate in GeoJSON positions "
                              "was found, but is not supported.");
    }

    // GeoJSON positions are in order longitude, latitude, altitude
    double lon = position.get(0).as_num();
    double lat = position.get(1).as_num();

    return lon_lat_point_t(lon, lat);
}

lon_lat_point_t extract_lon_lat_point(const datum_t &geojson) {
    if (geojson.get_field("type").as_str() != "Point") {
        throw geo_exception_t(
            strprintf("Expected geometry of type `Point` but found `%s`.",
                      geojson.get_field("type").as_str().to_std().c_str()));
    }

    const datum_t &coordinates = geojson.get_field("coordinates");

    return position_to_lon_lat_point(coordinates);
}

lon_lat_line_t extract_lon_lat_line(const ql::datum_t &geojson) {
    if (geojson.get_field("type").as_str() != "LineString") {
        throw geo_exception_t(
            strprintf("Expected geometry of type `LineString` but found `%s`.",
                      geojson.get_field("type").as_str().to_std().c_str()));
    }

    const datum_t &coordinates = geojson.get_field("coordinates");
    lon_lat_line_t result;
    result.reserve(coordinates.arr_size());
    for (size_t i = 0; i < coordinates.arr_size(); ++i) {
        result.push_back(position_to_lon_lat_point(coordinates.get(i)));
    }

    return result;
}

lon_lat_line_t extract_lon_lat_shell(const ql::datum_t &geojson) {
    if (geojson.get_field("type").as_str() != "Polygon") {
        throw geo_exception_t(
            strprintf("Expected geometry of type `Polygon` but found `%s`.",
                      geojson.get_field("type").as_str().to_std().c_str()));
    }

    const datum_t &coordinates = geojson.get_field("coordinates");
    if (coordinates.arr_size() < 1) {
        throw geo_exception_t("The polygon is empty. It must have at least "
                              "an outer shell.");
    }
    const datum_t ring_coordinates = coordinates.get(0);
    lon_lat_line_t result;
    result.reserve(ring_coordinates.arr_size());
    for (size_t i = 0; i < ring_coordinates.arr_size(); ++i) {
        result.push_back(position_to_lon_lat_point(ring_coordinates.get(i)));
    }

    return result;
}

// Parses a GeoJSON "Position" array
S2Point position_to_s2point(const datum_t &position) {
    lon_lat_point_t p = position_to_lon_lat_point(position);

    // Range checks (or S2 will terminate the process in debug mode).
    if(p.longitude < -180.0 || p.longitude > 180.0) {
        throw geo_range_exception_t(
            strprintf("Longitude must be between -180 and 180.  "
                      "Got %" PR_RECONSTRUCTABLE_DOUBLE ".", p.longitude));
    }
    if (p.longitude == 180.0) {
        p.longitude = -180.0;
    }
    if(p.latitude < -90.0 || p.latitude > 90.0) {
        throw geo_range_exception_t(
            strprintf("Latitude must be between -90 and 90.  "
                      "Got %" PR_RECONSTRUCTABLE_DOUBLE ".", p.latitude));
    }

    S2LatLng lat_lng(S1Angle::Degrees(p.latitude), S1Angle::Degrees(p.longitude));
    return lat_lng.ToPoint();
}

scoped_ptr_t<S2Point> coordinates_to_s2point(const datum_t &coords) {
    /* From the specs:
        "For type "Point", the "coordinates" member must be a single position."
    */
    S2Point pos = position_to_s2point(coords);
    return make_scoped<S2Point>(pos);
}

scoped_ptr_t<S2Polyline> coordinates_to_s2polyline(const datum_t &coords) {
    /* From the specs:
        "For type "LineString", the "coordinates" member must be an array of two
         or more positions."
    */
    if (coords.arr_size() < 2) {
        throw geo_exception_t(
            "GeoJSON LineString must have at least two positions.");
    }
    std::vector<S2Point> points;
    points.reserve(coords.arr_size());
    for (size_t i = 0; i < coords.arr_size(); ++i) {
        points.push_back(position_to_s2point(coords.get(i)));
    }
    if (!S2Polyline::IsValid(points)) {
        throw geo_exception_t(
            "Invalid LineString.  Are there antipodal or duplicate vertices?");
    }
    return make_scoped<S2Polyline>(points);
}

scoped_ptr_t<S2Loop> coordinates_to_s2loop(const datum_t &coords) {
    // Like a LineString, but must be connected
    if (coords.arr_size() < 4) {
        throw geo_exception_t(
            "GeoJSON LinearRing must have at least four positions.");
    }
    std::vector<S2Point> points;
    points.reserve(coords.arr_size());
    for (size_t i = 0; i < coords.arr_size(); ++i) {
        points.push_back(position_to_s2point(coords.get(i)));
    }
    if (points[0] != points[points.size()-1]) {
        throw geo_exception_t(
            "First and last vertex of GeoJSON LinearRing must be identical.");
    }
    // Drop the last point. S2Loop closes the loop implicitly.
    points.pop_back();

    // The second argument to IsValid is ignored...
    if (!S2Loop::IsValid(points, points.size())) {
        throw geo_exception_t(
            "Invalid LinearRing.  Are there antipodal or duplicate vertices? "
            "Is it self-intersecting?");
    }
    scoped_ptr_t<S2Loop> result(new S2Loop(points));
    // Normalize the loop
    result->Normalize();
    return result;
}

scoped_ptr_t<S2Polygon> coordinates_to_s2polygon(const datum_t &coords) {
    /* From the specs:
        "For type "Polygon", the "coordinates" member must be an array of LinearRing
         coordinate arrays. For Polygons with multiple rings, the first must be the
         exterior ring and any others must be interior rings or holes."
    */
    std::vector<scoped_ptr_t<S2Loop> > loops;
    loops.reserve(coords.arr_size());
    for (size_t i = 0; i < coords.arr_size(); ++i) {
        scoped_ptr_t<S2Loop> loop = coordinates_to_s2loop(coords.get(i));
        // Put the loop into the ptr_vector to avoid its destruction while
        // we are still building the polygon
        loops.push_back(std::move(loop));
    }

    // The first loop must be the outer shell, all other loops must be holes.
    // Invert the inner loops.
    for (size_t i = 1; i < loops.size(); ++i) {
        loops[i]->Invert();
    }

    // We use S2PolygonBuilder to automatically clean up identical edges and such
    S2PolygonBuilderOptions builder_opts = S2PolygonBuilderOptions::DIRECTED_XOR();
    // We don't have to run validation after every loop we add. It's enough to do
    // it once at the end.
    // However AssemblePolygon() would terminate the process if compiled in
    // debug mode (FLAGS_s2debug) upon encountering an invalid polygon, so we
    // have to validate early.
#ifndef NDEBUG
    builder_opts.set_validate(true);
#else
    builder_opts.set_validate(false);
#endif
    S2PolygonBuilder builder(builder_opts);
    for (size_t i = 0; i < loops.size(); ++i) {
        builder.AddLoop(loops[i].get());
    }

    scoped_ptr_t<S2Polygon> result(new S2Polygon());
    S2PolygonBuilder::EdgeList unused_edges;
    builder.AssemblePolygon(result.get(), &unused_edges);
    if (!unused_edges.empty() || !result->IsValid(geo::S2Polygon::validate_loops_t::NO)) {
        throw geo_exception_t(
            "Some edges in GeoJSON polygon could not be used.  Are they intersecting?");
    }

    return result;
}

void ensure_no_crs(const ql::datum_t &geojson) {
    const ql::datum_t &crs_field =
        geojson.get_field("crs", ql::throw_bool_t::NOTHROW);
    if (crs_field.has()) {
        if (crs_field.get_type() != ql::datum_t::R_NULL) {
            throw geo_exception_t("Non-default coordinate reference systems "
                                  "are not supported in GeoJSON objects.  "
                                  "Make sure the `crs` field of the geometry is "
                                  "null or non-existent.");
        }
    }
}

scoped_ptr_t<S2Point> to_s2point(const ql::datum_t &geojson) {
    datum_string_t type = geojson.get_field("type").as_str();
    datum_t coordinates = geojson.get_field("coordinates");
    if (type != "Point") {
        throw geo_exception_t(
            strprintf("Expected geometry of type `Point` but found `%s`.",
                      type.to_std().c_str()));
    }
    return coordinates_to_s2point(coordinates);
}

scoped_ptr_t<S2Polyline> to_s2polyline(const ql::datum_t &geojson) {
    datum_string_t type = geojson.get_field("type").as_str();
    datum_t coordinates = geojson.get_field("coordinates");
    if (type != "LineString") {
        throw geo_exception_t(
            strprintf("Expected geometry of type `LineString` but found `%s`.",
                      type.to_std().c_str()));
    }
    return coordinates_to_s2polyline(coordinates);
}

scoped_ptr_t<S2Polygon> to_s2polygon(const ql::datum_t &geojson) {
    datum_string_t type = geojson.get_field("type").as_str();
    datum_t coordinates = geojson.get_field("coordinates");
    if (type != "Polygon") {
        throw geo_exception_t(
            strprintf("Expected geometry of type `Polygon` but found `%s`.",
                      type.to_std().c_str()));
    }
    return coordinates_to_s2polygon(coordinates);
}

void validate_geojson(const ql::datum_t &geojson) {
    // Note that `visit_geojson()` already performs the majority of validations.

    ensure_no_crs(geojson);

    class validator_t : public s2_geo_visitor_t<void> {
    public:
        void on_point(UNUSED const S2Point &) {
            // Warning: If you change this function to perform any additional
            // checks on the S2Point, you need to be careful about maintaining
            // determinism of `point_term_t`.
            // See the note in the definition of `point_term_t` in
            // rdb_protocol/terms/geo.cc for details.
        }
        void on_line(UNUSED const S2Polyline &) { }
        void on_polygon(UNUSED const S2Polygon &) { }
    };
    validator_t validator;
    visit_geojson(&validator, geojson);
}

