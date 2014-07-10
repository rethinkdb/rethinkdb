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

dist_unit_t parse_dist_unit(const std::string &s) {
    if (s == "m") {
        return dist_unit_t::M;
    } else if (s == "km") {
        return dist_unit_t::KM;
    } else if (s == "mi") {
        return dist_unit_t::MI;
    } else if (s == "nm") {
        return dist_unit_t::NM;
    } else {
        throw geo_exception_t("Unrecognized distance unit: " + s);
    }
}

double unit_to_meters(dist_unit_t u) {
    switch (u) {
        case dist_unit_t::M: return 1.0;
        case dist_unit_t::KM: return 1000.0;
        case dist_unit_t::MI: return 1609.344;
        case dist_unit_t::NM: return 1852.0;
        case dist_unit_t::FT: return 0.3048;
        default: unreachable();
    }
}

double convert_dist_unit(double d, dist_unit_t from, dist_unit_t to) {
    // First convert to meters
    double conv_factor = unit_to_meters(from);

    // Then to the result unit
    conv_factor /= unit_to_meters(to);

    return d * conv_factor;
}
