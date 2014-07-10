// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "geo/primitives.hpp"

#include "geo/ellipsoid.hpp"
#include "geo/distances.hpp"


lat_lon_line_t build_circle(const lat_lon_point_t &center,
                            double radius,
                            unsigned int num_vertices,
                            const ellipsoid_spec_t &e) {
    lat_lon_line_t result;
    result.reserve(num_vertices + 1);

    for (unsigned int i = 0; i < num_vertices; ++i) {
        double azimuth = -180.0 + (360.0 / num_vertices * i);
        lat_lon_point_t v = karney_point_at_dist(center, radius, azimuth, e);
        result.push_back(v);
    }

    // Close the line
    if (num_vertices > 0) {
        result.push_back(result[0]);
    }

    return result;
}
