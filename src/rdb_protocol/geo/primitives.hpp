// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_PRIMITIVES_HPP_
#define RDB_PROTOCOL_GEO_PRIMITIVES_HPP_

#include "rdb_protocol/geo/lon_lat_types.hpp"

class ellipsoid_spec_t;

// Constructs a circle with of radius `radius` (in m)
lon_lat_line_t build_circle(
        const lon_lat_point_t &center,
        double radius,
        unsigned int num_vertices,
        const ellipsoid_spec_t &e);

// The resulting polygon has an incircle of at least min_inradius around center.
lon_lat_line_t build_polygon_with_inradius_at_least(
        const lon_lat_point_t &center,
        double min_inradius,
        unsigned int num_vertices,
        const ellipsoid_spec_t &e);

// The resulting polygon has an excircle of no more than max_exradius around center.
lon_lat_line_t build_polygon_with_exradius_at_most(
        const lon_lat_point_t &center,
        double max_exradius,
        unsigned int num_vertices,
        const ellipsoid_spec_t &e);

#endif  // RDB_PROTOCOL_GEO_PRIMITIVES_HPP_
