// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "geo/distances.hpp"

#include "geo/ellipsoid.hpp"
#include "geo/exceptions.hpp"
#include "geo/karney/geodesic.h"

// The inverse problem of Vincenty's formulas
// See http://en.wikipedia.org/w/index.php?title=Vincenty%27s_formulae&oldid=607167872
double karney_distance(const lat_lon_point_t &p1,
                       const lat_lon_point_t &p2,
                       const ellipsoid_spec_t &e) {
    struct geod_geodesic g;
    geod_init(&g, e.equator_radius(), e.flattening());

    double dist;
    double azi1, azi2;
    geod_inverse(&g, p1.first, p1.second, p2.first, p2.second, &dist, &azi1, &azi2);

    return dist;
}
