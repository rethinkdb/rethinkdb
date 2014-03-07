// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>


#include <geometry_test_common.hpp>

#include <algorithms/test_intersects.hpp>

#include <boost/geometry.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

template <typename P>
void test_all()
{
    typedef bg::model::polygon<P> polygon;
    typedef bg::model::multi_polygon<polygon> mp;

    test_geometry<mp, mp>("MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
            "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
        true);

    test_geometry<P, mp>("POINT(0 0)",
        "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
        true);

}

int test_main(int, char* [])
{
    //test_all<bg::model::d2::point_xy<float> >();
    test_all<bg::model::d2::point_xy<double> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}

