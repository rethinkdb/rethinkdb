// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/geo/primitives.hpp"

#include <algorithm>
#include <cmath>

#include "rdb_protocol/geo/ellipsoid.hpp"
#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/geo/distances.hpp"
#include "rdb_protocol/datum.hpp" // For PR_RECONSTRUCTABLE_DOUBLE


lon_lat_line_t build_circle(
        const lon_lat_point_t &center,
        double radius,
        unsigned int num_vertices,
        const ellipsoid_spec_t &e) {

    if (radius <= 0.0) {
        throw geo_range_exception_t("Radius must be positive");
    }
    const double max_radius = 0.5 * M_PI * std::min(e.equator_radius(), e.poles_radius());
    if (radius >= max_radius) {
        throw geo_range_exception_t(
            strprintf("Radius must be smaller than a quarter of the circumference "
                      "along the minor axis of the reference ellipsoid.  "
                      "Got %" PR_RECONSTRUCTABLE_DOUBLE "m, but must be smaller "
                      "than %" PR_RECONSTRUCTABLE_DOUBLE "m.",
                      radius, max_radius));
    }

    lon_lat_line_t result;
    result.reserve(num_vertices + 1);

    for (unsigned int i = 0; i < num_vertices; ++i) {
        double azimuth = -180.0 + (360.0 / num_vertices * i);
        lon_lat_point_t v = geodesic_point_at_dist(center, radius, azimuth, e);
        result.push_back(v);
    }

    // Close the line
    if (num_vertices > 0) {
        result.push_back(result[0]);
    }

    return result;
}

/* WARNING: This function is used by nearest_traversal_cb_t::init_query_geometry()
 * and must provide some strict guarantees. Read the notes in that function before
 * modifying this. */
lon_lat_line_t build_polygon_with_inradius_at_least(
        const lon_lat_point_t &center,
        double min_inradius,
        unsigned int num_vertices,
        const ellipsoid_spec_t &e) {

    // Make the radius slightly larger, just to be sure given limited numeric
    // precision and such.
    const double leeway_factor = 1.01;
    double r = min_inradius * leeway_factor;

    // How do we get a lower bound on the inradius?
    // For radii that are very small compared to the circumference of the sphere,
    // everything behaves like in Euclidean geometry, and we can use the formula
    // `exradius = inradius / cos(pi / num_vertices)` for a regular polygon.
    // On the other extreme, if the radius is a quarter of the circumference of
    // the reference sphere, the exradius of the polygon will actually be the
    // same as the inradius. In that case, our formula overestimates the exradius,
    // which is safe.
    // When having an ellipsoid, we additionally apply a correction factor to
    // (over-)compensate for potential distortions of the distances involved.
    // TODO (daniel): Open question: Would this still be required if we didn't
    //   assume a sphere when computing polygon intersection (through S2)?
    //   Not that it matters, since we do, but it would be interesting.
    // TODO (daniel): While this has been shown to work empirically,
    //   it would be nice to get a formal proof for those things. Especially
    //   for the ellipsoid correction
    double max_distortion = std::max(e.equator_radius(), e.poles_radius()) /
        std::min(e.equator_radius(), e.poles_radius());
    double ex_r = r / std::cos(M_PI / num_vertices) * max_distortion;

    return build_circle(center, ex_r, num_vertices, e);
}

/* WARNING: This function is used by nearest_traversal_cb_t::init_query_geometry()
 * and must provide some strict guarantees. Read the notes in that function before
 * modifying this. */
lon_lat_line_t build_polygon_with_exradius_at_most(
        const lon_lat_point_t &center,
        double max_exradius,
        unsigned int num_vertices,
        const ellipsoid_spec_t &e) {

    // Make the radius slightly smaller, just to be sure given limited numeric
    // precision and such.
    const double leeway_factor = 0.99;
    double r = max_exradius * leeway_factor;

    // `r` is an upper limit on the exradius of the resulting circle, even in
    // spherical geometry. In the extreme case, of r being a quarter of the
    // circumference of the sphere, the resulting polygon will be *exactly* a
    // circle with radius r. For very small r on the other hand the lines between
    // the vertices of the polygon will behave like in Euclidean space.

    return build_circle(center, r, num_vertices, e);
}
