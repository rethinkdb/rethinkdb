// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithms/test_length.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/std_pair_as_segment.hpp>

#include <test_geometries/all_custom_linestring.hpp>
#include <test_geometries/wrapped_boost_array.hpp>


template <typename P>
void test_all()
{
    // 3-4-5 triangle
    test_geometry<std::pair<P, P> >("LINESTRING(0 0,3 4)", 5);

    // 3-4-5 plus 1-1
    test_geometry<bg::model::linestring<P> >("LINESTRING(0 0,3 4,4 3)", 5 + sqrt(2.0));
    test_geometry<all_custom_linestring<P> >("LINESTRING(0 0,3 4,4 3)", 5 + sqrt(2.0));
    test_geometry<test::wrapped_boost_array<P, 3> >("LINESTRING(0 0,3 4,4 3)", 5 + sqrt(2.0));

    // Geometries with length zero
    test_geometry<P>("POINT(0 0)", 0);
    test_geometry<bg::model::polygon<P> >("POLYGON((0 0,0 1,1 1,1 0,0 0))", 0);
}

template <typename P>
void test_empty_input()
{
    test_empty_input(bg::model::linestring<P>());
}

int test_main(int, char* [])
{
    test_all<bg::model::d2::point_xy<int> >();
    test_all<bg::model::d2::point_xy<float> >();
    test_all<bg::model::d2::point_xy<double> >();

#if defined(HAVE_TTMATH)
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    // test_empty_input<bg::model::d2::point_xy<int> >();

    return 0;
}
