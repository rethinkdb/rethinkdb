// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "geo/primitives.hpp"

#include <algorithm>
#include <cmath>

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
        unsigned int num_vertices,
        const ellipsoid_spec_t &e) {

    // Make the radius slightly larger, just to be sure given limited numeric
    // precision and such.
    const double leeway_factor = 1.01;
    double r = min_inradius * leeway_factor;

    double max_distortion = std::max(e.equator_radius(), e.poles_radius()) /
        std::min(e.equator_radius(), e.poles_radius());
    // /cos(M_PI / num_vertices) is the formula for regular polygons in
    // Euclidean space http://mathworld.wolfram.com/RegularPolygon.html
    // It appears that it also works on a sphere's surface. We have to apply a
    // correction factor when being on the surface of an ellipsoid though.
    // TODO! This is unproven speculation. Check this stuff.
    double ex_r = r / std::cos(M_PI / num_vertices) * max_distortion;

    return build_circle(center, ex_r, num_vertices, e);
}

/* WARNING: This function is used by nearest_traversal_cb_t::init_query_geometry()
 * and must provide some strict guarantees. Read the notes in that function before
 * modifying this. */
lat_lon_line_t build_polygon_with_exradius_at_most(
        const lat_lon_point_t &center,
        double max_exradius,
        unsigned int num_vertices,
        const ellipsoid_spec_t &e) {

    // Make the radius slightly smaller, just to be sure given limited numeric
    // precision and such.
    const double leeway_factor = 0.99;
    double r = max_exradius * leeway_factor;

    return build_circle(center, r, num_vertices, e);
}
