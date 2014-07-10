// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "geo/distances.hpp"

#include "geo/ellipsoid.hpp"
#include "geo/exceptions.hpp"
#include "geo/karney/geodesic.h"

double karney_distance(const lat_lon_point_t &p1,
                       const lat_lon_point_t &p2,
                       const ellipsoid_spec_t &e) {
    struct geod_geodesic g;
    geod_init(&g, e.equator_radius(), e.flattening());

    double dist;
    geod_inverse(&g, p1.first, p1.second, p2.first, p2.second, &dist, NULL, NULL);

    return dist;
}

lat_lon_point_t karney_point_at_dist(const lat_lon_point_t &p,
                                     double dist,
                                     double azimuth,
                                     const ellipsoid_spec_t &e) {
    struct geod_geodesic g;
    geod_init(&g, e.equator_radius(), e.flattening());

    double lat, lon;
    geod_direct(&g, p.first, p.second, azimuth, dist, &lat, &lon, NULL);

    return lat_lon_point_t(lat, lon);
}
