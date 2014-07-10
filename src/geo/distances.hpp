// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef GEO_DISTANCES_HPP_
#define GEO_DISTANCES_HPP_

#include <utility>

class ellipsoid_spec_t;

// TODO!
typedef std::pair<double, double> lat_lon_point_t;

// Returns the ellipsoidal distance between p1 and p2 on e (in meters).
// (solves the inverse geodesic problem)
double karney_distance(const lat_lon_point_t &p1,
                       const lat_lon_point_t &p2,
                       const ellipsoid_spec_t &e);

// Returns a point at distance `dist` (in meters) of `p` in direction `azimuth`
// (in degrees between -180 and 180)
// (solves the direct geodesic problem)
lat_lon_point_t karney_point_at_dist(const lat_lon_point_t &p,
                                     double dist,
                                     double azimuth,
                                     const ellipsoid_spec_t &e);

#endif  // GEO_DISTANCES_HPP_
