// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef GEO_PRIMITIVES_HPP_
#define GEO_PRIMITIVES_HPP_

#include <utility>
#include <vector>

class ellipsoid_spec_t;

// TODO!
typedef std::pair<double, double> lat_lon_point_t;
typedef std::vector<lat_lon_point_t> lat_lon_line_t;


// Constructs a circle with of radius `radius` (in m)
lat_lon_line_t build_circle(const lat_lon_point_t &center,
                            double radius,
                            unsigned int num_vertices,
                            const ellipsoid_spec_t &e);

// TODO! Rectangle

#endif  // GEO_PRIMITIVES_HPP_
