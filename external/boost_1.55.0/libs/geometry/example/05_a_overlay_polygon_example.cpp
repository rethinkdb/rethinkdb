// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Polygon Overlay Example

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <boost/foreach.hpp>


#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>

#if defined(HAVE_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#endif

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)


int main(void)
{
    namespace bg = boost::geometry;

    typedef bg::model::d2::point_xy<double> point_2d;
    typedef bg::model::polygon<point_2d> polygon_2d;


#if defined(HAVE_SVG)
    std::ofstream stream("05_a_intersection_polygon_example.svg");
    bg::svg_mapper<point_2d> svg(stream, 500, 500);
#endif

    // Define a polygons and fill the outer rings.
    polygon_2d a;
    {
        const double c[][2] = {
            {160, 330}, {60, 260}, {20, 150}, {60, 40}, {190, 20}, {270, 130}, {260, 250}, {160, 330}
        };
        bg::assign_points(a, c);
    }
    bg::correct(a);
    std::cout << "A: " << bg::dsv(a) << std::endl;

    polygon_2d b;
    {
        const double c[][2] = {
            {300, 330}, {190, 270}, {150, 170}, {150, 110}, {250, 30}, {380, 50}, {380, 250}, {300, 330}
        };
        bg::assign_points(b, c);
    }
    bg::correct(b);
    std::cout << "B: " << bg::dsv(b) << std::endl;
#if defined(HAVE_SVG)
    svg.add(a);
    svg.add(b);

    svg.map(a, "opacity:0.6;fill:rgb(0,255,0);");
    svg.map(b, "opacity:0.6;fill:rgb(0,0,255);");
#endif


    // Calculate interesection(s)
    std::vector<polygon_2d> intersection;
    bg::intersection(a, b, intersection);

    std::cout << "Intersection of polygons A and B" << std::endl;
    BOOST_FOREACH(polygon_2d const& polygon, intersection)
    {
        std::cout << bg::dsv(polygon) << std::endl;
#if defined(HAVE_SVG)
        svg.map(polygon, "opacity:0.5;fill:none;stroke:rgb(255,0,0);stroke-width:6");
#endif
    }

    return 0;
}
