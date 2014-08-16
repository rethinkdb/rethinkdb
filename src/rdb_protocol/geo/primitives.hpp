// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_PRIMITIVES_HPP_
#define RDB_PROTOCOL_GEO_PRIMITIVES_HPP_

#include "rdb_protocol/geo/lat_lon_types.hpp"

class ellipsoid_spec_t;

// Constructs a circle with of radius `radius` (in m)
lat_lon_line_t build_circle(
        const lat_lon_point_t &center,
        double radius,
        unsigned int num_vertices,
        const ellipsoid_spec_t &e);

// The resulting polygon has an incircle of at least min_inradius around center.
lat_lon_line_t build_polygon_with_inradius_at_least(
        const lat_lon_point_t &center,
        double min_inradius,
        unsigned int num_vertices,
        const ellipsoid_spec_t &e);

// The resulting polygon has an excircle of no more than max_exradius around center.
lat_lon_line_t build_polygon_with_exradius_at_most(
        const lat_lon_point_t &center,
        double max_exradius,
        unsigned int num_vertices,
        const ellipsoid_spec_t &e);

#endif  // RDB_PROTOCOL_GEO_PRIMITIVES_HPP_
