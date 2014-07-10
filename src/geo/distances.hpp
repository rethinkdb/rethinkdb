// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef GEO_DISTANCES_HPP_
#define GEO_DISTANCES_HPP_

#include <utility>

class ellipsoid_spec_t;

typedef std::pair<double, double> lat_lon_point_t;

// Returns the ellispoidal distance between p1 and p2 on e (in meters).
double karney_distance(const lat_lon_point_t &p1,
                       const lat_lon_point_t &p2,
                       const ellipsoid_spec_t &e);

// TODO! Implement the direct problem so we can construct surface circles

#endif  // GEO_DISTANCES_HPP_
