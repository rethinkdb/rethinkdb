// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Linestring Polygon Overlay Example

// NOTE: this example is obsolete. Boost.Geometry can now
// overlay linestrings/polygons.
// This sample will be removed in next version.

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <boost/foreach.hpp>


#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>

#if defined(HAVE_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#endif

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian);


int main(void)
{
    namespace bg = boost::geometry;

    typedef bg::model::d2::point_xy<double> point_2d;

    bg::model::linestring<point_2d> ls;
    {
        const double c[][2] = { {0, 1}, {2, 5}, {5, 3} };
        bg::assign_points(ls, c);
    }

    bg::model::polygon<point_2d> p;
    {
        const double c[][2] = { {3, 0}, {0, 3}, {4, 5}, {3, 0} };
        bg::assign_points(p, c);
    }
    bg::correct(p);

#if defined(HAVE_SVG)
    // Create SVG-mapper
    std::ofstream stream("05_b_overlay_linestring_polygon_example.svg");
    bg::svg_mapper<point_2d> svg(stream, 500, 500);
    // Determine extend by adding geometries
    svg.add(p);
    svg.add(ls);
    // Map geometries
    svg.map(ls, "opacity:0.6;stroke:rgb(255,0,0);stroke-width:2;");
    svg.map(p, "opacity:0.6;fill:rgb(0,0,255);");
#endif

    // Calculate intersection points (turn points)
    typedef bg::detail::overlay::turn_info<point_2d> turn_info;
    std::vector<turn_info> turns;
    bg::detail::get_turns::no_interrupt_policy policy;
    bg::get_turns<false, false, bg::detail::overlay::assign_null_policy>(ls, p, turns, policy);

    std::cout << "Intersection of linestring/polygon" << std::endl;
    BOOST_FOREACH(turn_info const& turn, turns)
    {
        std::string action = "intersecting";
        if (turn.operations[0].operation
                == bg::detail::overlay::operation_intersection)
        {
            action = "entering";
        }
        else if (turn.operations[0].operation
                == bg::detail::overlay::operation_union)
        {
            action = "leaving";

        }
        std::cout << action << " polygon at " << bg::dsv(turn.point) << std::endl;
#if defined(HAVE_SVG)
        svg.map(turn.point, "fill:rgb(255,128,0);stroke:rgb(0,0,100);stroke-width:1");
        svg.text(turn.point, action, "fill:rgb(0,0,0);font-family:Arial;font-size:10px");
#endif
    }

    return 0;
}
