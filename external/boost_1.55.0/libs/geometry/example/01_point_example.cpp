// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Point Example - showing different type of points

#include <iostream>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_array.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon/point.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


int main()
{
    using namespace boost::geometry;

    // Boost.Geometry contains several point types:
    // 1: its own generic type
    model::point<double, 2, cs::cartesian> pt1;

    // 2: its own type targetted to Cartesian (x,y) coordinates
    model::d2::point_xy<double> pt2;

    // 3: it supports Boost tuple's
    boost::tuple<double, double> pt3;

    // 4: it supports normal arrays
    double pt4[2];

    // 5: it supports arrays-as-points from Boost.Array
    boost::array<double, 2> pt5;

    // 6: it supports points from Boost.Polygon
    boost::polygon::point_data<double> pt6;

    // 7: in the past there was a typedef point_2d
    //    But users are now supposted to do that themselves:
    typedef model::d2::point_xy<double> point_2d;
    point_2d pt7;


    // 7: there are more variants, and you can create your own.
    //    (see therefore the custom_point example)

    // All these types are handled the same way. We show here
    // assigning them and calculating distances.
    assign_values(pt1, 1, 1);
    assign_values(pt2, 2, 2);
    assign_values(pt3, 3, 3);
    assign_values(pt4, 4, 4);
    assign_values(pt5, 5, 5);
    assign_values(pt6, 6, 6);
    assign_values(pt7, 7, 7);


    double d1 = distance(pt1, pt2);
    double d2 = distance(pt3, pt4);
    double d3 = distance(pt5, pt6);
    std::cout << "Distances: " 
        << d1 << " and " << d2 << " and " << d3 << std::endl;

    // (in case you didn't note, distances can be calculated
    //  from points with different point-types)


    // Several ways of construction and setting point values
    // 1: default, empty constructor, causing no initialization at all
    model::d2::point_xy<double> p1;

    // 2: as shown above, assign_values
    model::d2::point_xy<double> p2;
    assign_values(p2, 1, 1);

    // 3: using "set" function
    //    set uses the concepts behind, such that it can be applied for
    //    every point-type (like assign_values)
    model::d2::point_xy<double> p3;
    set<0>(p3, 1);
    set<1>(p3, 1);
    // set<2>(p3, 1); //will result in compile-error


    // 3: for any point type, and other geometry objects:
    //    there is the "make" object generator
    //    (this one requires to specify the point-type).
    model::d2::point_xy<double> p4 = make<model::d2::point_xy<double> >(1,1);


    // 5: for the d2::point_xy<...> type only: constructor with two values
    model::d2::point_xy<double> p5(1,1);

    // 6: for boost tuples you can of course use make_tuple


    // Some ways of getting point values

    // 1: using the "get" function following the concepts behind
    std::cout << get<0>(p2) << "," << get<1>(p2) << std::endl;

    // 2: for point_xy only
    std::cout << p2.x() << "," << p2.y() << std::endl;

    // 3: using boost-tuples you of course can boost-tuple-methods
    std::cout << pt3.get<0>() << "," << pt3.get<1>() << std::endl;

    // 4: Boost.Geometry supports various output formats, e.g. DSV
    //    (delimiter separated values)
    std::cout << dsv(pt3) << std::endl;

    // There are 3-dimensional points too
    model::point<double, 3, cs::cartesian> d3a, d3b;
    assign_values(d3a, 1, 2, 3);
    assign_values(d3b, 4, 5, 6);
    d3 = distance(d3a, d3b);



    // Other examples show other types of points, geometries and more algorithms

    return 0;
}
