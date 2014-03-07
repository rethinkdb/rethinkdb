// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Polygon Example

#include <algorithm> // for reverse, unique
#include <iostream>
#include <string>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)


std::string boolstr(bool v)
{
    return v ? "true" : "false";
}

int main(void)
{
    using namespace boost::geometry;

    typedef model::d2::point_xy<double> point_2d;
    typedef model::polygon<point_2d> polygon_2d;
    typedef model::box<point_2d> box_2d;

    // Define a polygon and fill the outer ring.
    // In most cases you will read it from a file or database
    polygon_2d poly;
    {
        const double coor[][2] = {
            {2.0, 1.3}, {2.4, 1.7}, {2.8, 1.8}, {3.4, 1.2}, {3.7, 1.6},
            {3.4, 2.0}, {4.1, 3.0}, {5.3, 2.6}, {5.4, 1.2}, {4.9, 0.8}, {2.9, 0.7},
            {2.0, 1.3} // closing point is opening point
            };
        assign_points(poly, coor);
    }

    // Polygons should be closed, and directed clockwise. If you're not sure if that is the case,
    // call the correct algorithm
    correct(poly);

    // Polygons can be streamed as text
    // (or more precisely: as DSV (delimiter separated values))
    std::cout << dsv(poly) << std::endl;

    // As with lines, bounding box of polygons can be calculated
    box_2d b;
    envelope(poly, b);
    std::cout << dsv(b) << std::endl;

    // The area of the polygon can be calulated
    std::cout << "area: " << area(poly) << std::endl;

    // And the centroid, which is the center of gravity
    point_2d cent;
    centroid(poly, cent);
    std::cout << "centroid: " << dsv(cent) << std::endl;


    // The number of points can be requested per ring (using .size())
    // or per polygon (using num_points)
    std::cout << "number of points in outer ring: " << poly.outer().size() << std::endl;

    // Polygons can have one or more inner rings, also called holes, islands, interior rings.
    // Let's add one
    {
        poly.inners().resize(1);
        model::ring<point_2d>& inner = poly.inners().back();

        const double coor[][2] = { {4.0, 2.0}, {4.2, 1.4}, {4.8, 1.9}, {4.4, 2.2}, {4.0, 2.0} };
        assign_points(inner, coor);
    }

    correct(poly);

    std::cout << "with inner ring:" << dsv(poly) << std::endl;
    // The area of the polygon is changed of course
    std::cout << "new area of polygon: " << area(poly) << std::endl;
    centroid(poly, cent);
    std::cout << "new centroid: " << dsv(cent) << std::endl;

    // You can test whether points are within a polygon
    std::cout << "point in polygon:"
        << " p1: "  << boolstr(within(make<point_2d>(3.0, 2.0), poly))
        << " p2: "  << boolstr(within(make<point_2d>(3.7, 2.0), poly))
        << " p3: "  << boolstr(within(make<point_2d>(4.4, 2.0), poly))
        << std::endl;

    // As with linestrings and points, you can derive from polygon to add, for example,
    // fill color and stroke color. Or SRID (spatial reference ID). Or Z-value. Or a property map.
    // We don't show this here.

    // Clip the polygon using a box
    box_2d cb(make<point_2d>(1.5, 1.5), make<point_2d>(4.5, 2.5));
    typedef std::vector<polygon_2d> polygon_list;
    polygon_list v;

    intersection(cb, poly, v);
    std::cout << "Clipped output polygons" << std::endl;
    for (polygon_list::const_iterator it = v.begin(); it != v.end(); ++it)
    {
        std::cout << dsv(*it) << std::endl;
    }

    typedef model::multi_polygon<polygon_2d> polygon_set;
    polygon_set ps;
    union_(cb, poly, ps);

    polygon_2d hull;
    convex_hull(poly, hull);
    std::cout << "Convex hull:" << dsv(hull) << std::endl;

    // If you really want:
    //   You don't have to use a vector, you can define a polygon with a deque
    //   You can specify the container for the points and for the inner rings independantly

    typedef model::polygon<point_2d, true, true, std::deque, std::deque> deque_polygon;
    deque_polygon poly2;
    ring_type<deque_polygon>::type& ring = exterior_ring(poly2);
    append(ring, make<point_2d>(2.8, 1.9));
    append(ring, make<point_2d>(2.9, 2.4));
    append(ring, make<point_2d>(3.3, 2.2));
    append(ring, make<point_2d>(3.2, 1.8));
    append(ring, make<point_2d>(2.8, 1.9));
    std::cout << dsv(poly2) << std::endl;

    return 0;
}
