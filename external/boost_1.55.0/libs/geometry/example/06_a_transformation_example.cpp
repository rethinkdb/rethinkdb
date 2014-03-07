// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Transformation Example

#include <iostream>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)


int main()
{
    using namespace boost::geometry;

    typedef model::d2::point_xy<double> point_2d;
    point_2d p(1, 1);
    point_2d p2;

    // Example: translate a point over (5,5)
    strategy::transform::translate_transformer<double, 2, 2> translate(5, 5);

    transform(p, p2, translate);
    std::cout << "transformed point " << boost::geometry::dsv(p2) << std::endl;

    // Transform a polygon
    model::polygon<point_2d> poly, poly2;
    const double coor[][2] = { {0, 0}, {0, 7}, {2, 2}, {2, 0}, {0, 0} };
    // note that for this syntax you have to include the two
    // include files above (c_array.hpp)
    assign_points(poly, coor);
    //read_wkt("POLYGON((0 0,0 7,4 2,2 0,0 0))", poly);
    transform(poly, poly2, translate);

    std::cout << "source      polygon " << boost::geometry::dsv(poly) << std::endl;
    std::cout << "transformed polygon " << boost::geometry::dsv(poly2) << std::endl;

    // Many more transformations are possible:
    // - from Cartesian to Spherical coordinate systems and back
    // - from Cartesian to Cartesian (mapping, affine transformations) and back (inverse)
    // - Map Projections
    // - from Degree to Radian and back in spherical_equatorial or geographic coordinate systems

    return 0;
}
