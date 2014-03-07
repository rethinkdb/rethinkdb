// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Example combining Boost.Geometry with Boost.Assign and Boost.Range and Boost.Tuple

#include <iostream>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

#include <boost/assign.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian);


int main(void)
{
    using namespace boost::geometry;
    using namespace boost::assign;

    {
        typedef model::d2::point_xy<double> point;
        typedef model::polygon<point> polygon;
        typedef model::linestring<point> linestring;

        // Boost.Assign automatically works for linestring, rings, multi-geometries
        // It works because these are std:: containers

        // Using Boost.Assign operator +=
        linestring ls1;
        ls1 += point(1,2);
        ls1 += point(3,4), point(5,6), point(7,8);
        std::cout << dsv(ls1) << std::endl;

        // Using Boost.Assign operator()
        linestring ls2;
        push_back(ls2)(point(1, 2))(point(3, 4));
        std::cout << dsv(ls2) << std::endl;

        // Using Boost.Assign list_of
        linestring ls3 = list_of(point(1,2))(point(3,4));
        std::cout << dsv(ls3) << std::endl;

        // Using Boost.Assign + Boost.Range
        linestring ls4;
        push_back(ls4)(point(0, 0)).range(ls2).range(ls3);
        std::cout << dsv(ls4) << std::endl;

        // For a ring, it is similar to a linestring.
        // For a multi-point or multi-linestring, it is also similar
        // For a polygon, take the exterior ring or one of the interiors
        polygon p;
        push_back(exterior_ring(p))
            (point(0, 0))
            (point(0, 2))
            (point(2, 2))
            (point(2, 0))
            (point(0, 0))
            ;

        std::cout << dsv(p) << std::endl;
    }

    {
        // It is convenient to combine Boost.Assign on a geometry (e.g. polygon) with tuples.
        typedef model::polygon<boost::tuple<double,double> > polygon;

        polygon p;
        exterior_ring(p) = tuple_list_of(0, 0)(0, 5)(5, 5)(5, 0)(0, 0);

        std::cout << dsv(p) << std::endl;

        // And let it work on the interior_rings as well
        push_back(interior_rings(p))
            (tuple_list_of(1, 1)(2, 1)(2, 2)(1, 2)(1, 1))
            (tuple_list_of(3, 3)(4, 3)(4, 4)(3, 4)(3, 3))
            ;

        std::cout << "Area of " << dsv(p) << " is " << area(p) << std::endl;
    }


    return 0;
}
