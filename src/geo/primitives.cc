// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "geo/primitives.hpp"

#include <algorithm>

#include "geo/ellipsoid.hpp"
#include "geo/exceptions.hpp"
#include "geo/distances.hpp"


lat_lon_line_t build_circle(
        const lat_lon_point_t &center,
        double radius,
        unsigned int num_vertices,
        const ellipsoid_spec_t &e) {

    if (radius <= 0.0) {
        throw geo_exception_t("Radius must be positive");
    }
    if (radius >= std::min(e.equator_radius(), e.poles_radius())) {
        throw geo_exception_t(
            strprintf("Radius must be smaller than the minimal radius of the "
                      "reference ellipsoid (which is %f).",
                      std::min(e.equator_radius(), e.poles_radius())));
    }

    lat_lon_line_t result;
    result.reserve(num_vertices + 1);

    for (unsigned int i = 0; i < num_vertices; ++i) {
        double azimuth = -180.0 + (360.0 / num_vertices * i);
        lat_lon_point_t v = geodesic_point_at_dist(center, radius, azimuth, e);
        result.push_back(v);
    }

    // Close the line
    if (num_vertices > 0) {
        result.push_back(result[0]);
    }

    return result;
}

lat_lon_line_t build_rectangle(
        const lat_lon_point_t &base,
        const lat_lon_point_t &opposite) {

    if (base.first == opposite.first) {
        throw geo_exception_t("Opposite and base points have the same latitude.");
    }
    if (base.second == opposite.second) {
        throw geo_exception_t("Opposite and base points have the same longitude.");
    }

    lat_lon_line_t result;
    result.reserve(5);
    result.push_back(base);
    result.push_back(lat_lon_point_t(base.first, opposite.second));
    result.push_back(opposite);
    result.push_back(lat_lon_point_t(opposite.first, base.second));
    result.push_back(base);
    return result;
}

/* WARNING: This function is used by nearest_traversal_cb_t::init_query_geometry()
 * and must provide some strict guarantees. Read the notes in that function before
 * modifying this. */
lat_lon_line_t build_polygon_with_inradius_at_least(
        const lat_lon_point_t &center,
        double min_inradius,
        const ellipsoid_spec_t &e) {
    // TODO! Check radius range

    // Make the radius slightly larger, just to be sure given limited numeric
    // precision and such.
    const double leeway_factor = 1.01;
    double r = min_inradius * leeway_factor;

    // Here's a trick to construct a rectangle with the given inradius:
    // We first compute points that are in the center of each of the
    // rectangle's edges. Then we compute the vertices from those.

    // TODO! Use a finer shape than a rectangle.
    // Assume azimuth 0 is north, 90 is east
    const lat_lon_point_t north = geodesic_point_at_dist(center, r, 0, e);
    const lat_lon_point_t south = geodesic_point_at_dist(center, r, 180, e);
    const lat_lon_point_t west = geodesic_point_at_dist(center, r, -90, e);
    const lat_lon_point_t east = geodesic_point_at_dist(center, r, 90, e);
    // TODO!
    const double max_lat = east.first;
    const double min_lat = west.first;
    const double max_lon = south.second;
    const double min_lon = north.second;

    fprintf(stderr, "r: %f lat: [%f, %f] lon: [%f, %f]\n", r, min_lat, max_lat, min_lon, max_lon);

    lat_lon_line_t result;
    result.reserve(5);
    result.push_back(lat_lon_point_t(north.first, east.second));
    result.push_back(lat_lon_point_t(south.first, min_lon));
    result.push_back(lat_lon_point_t(min_lat, min_lon));
    result.push_back(lat_lon_point_t(min_lat, max_lon));
    result.push_back(lat_lon_point_t(max_lat, max_lon));

    return result;
}

/* WARNING: This function is used by nearest_traversal_cb_t::init_query_geometry()
 * and must provide some strict guarantees. Read the notes in that function before
 * modifying this. */
lat_lon_line_t build_polygon_with_exradius_at_most(
        const lat_lon_point_t &center,
        double max_exradius,
        const ellipsoid_spec_t &e) {
    // TODO! Check radius range

    // Make the radius slightly smaller, just to be sure given limited numeric
    // precision and such.
    const double leeway_factor = 0.99;
    double r = max_exradius * leeway_factor;

    // TODO! Use a finer shape than a rectangle
    lat_lon_line_t result;
    result.reserve(5);
    result.push_back(geodesic_point_at_dist(center, r, 0, e));
    result.push_back(geodesic_point_at_dist(center, r, 180, e));
    result.push_back(geodesic_point_at_dist(center, r, -90, e));
    result.push_back(geodesic_point_at_dist(center, r, 90, e));
    result.push_back(result[0]);

    return result;
}
