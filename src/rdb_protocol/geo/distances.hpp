// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_DISTANCES_HPP_
#define RDB_PROTOCOL_GEO_DISTANCES_HPP_

#include <string>
#include <utility>

#include "containers/counted.hpp"
#include "rdb_protocol/geo/lon_lat_types.hpp"
#include "rdb_protocol/geo/s2/util/math/vector3.h"

namespace geo {
typedef Vector3_d S2Point;
}

class ellipsoid_spec_t;
namespace ql {
class datum_t;
}

// Returns the ellipsoidal distance between p1 and p2 on e (in meters).
// (solves the inverse geodesic problem)
double geodesic_distance(const lon_lat_point_t &p1,
                         const lon_lat_point_t &p2,
                         const ellipsoid_spec_t &e);
double geodesic_distance(const geo::S2Point &p,
                         const ql::datum_t &g,
                         const ellipsoid_spec_t &e);

// Returns a point at distance `dist` (in meters) of `p` in direction `azimuth`
// (in degrees between -180 and 180)
// (solves the direct geodesic problem)
lon_lat_point_t geodesic_point_at_dist(const lon_lat_point_t &p,
                                       double dist,
                                       double azimuth,
                                       const ellipsoid_spec_t &e);

// M meters
// KM kilometers
// MI international miles
// NM nautical miles
// FT international feet
enum class dist_unit_t { M, KM, MI, NM, FT };

dist_unit_t parse_dist_unit(const std::string &s);

double convert_dist_unit(double d, dist_unit_t from, dist_unit_t to);

#endif  // RDB_PROTOCOL_GEO_DISTANCES_HPP_
