// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef GEO_PRIMITIVES_HPP_
#define GEO_PRIMITIVES_HPP_

#include "geo/lat_lon_types.hpp"

class ellipsoid_spec_t;

// Constructs a circle with of radius `radius` (in m)
lat_lon_line_t build_circle(const lat_lon_point_t &center,
                            double radius,
                            unsigned int num_vertices,
                            const ellipsoid_spec_t &e);

// Constructs a rectangle-like line
lat_lon_line_t build_rectangle(const lat_lon_point_t &base,
                               const lat_lon_point_t &opposite);

#endif  // GEO_PRIMITIVES_HPP_
