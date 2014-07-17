// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "geo/primitives.hpp"

#include <algorithm>

#include "geo/ellipsoid.hpp"
#include "geo/exceptions.hpp"
#include "geo/distances.hpp"


lat_lon_line_t build_circle(const lat_lon_point_t &center,
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

lat_lon_line_t build_rectangle(const lat_lon_point_t &base,
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
