// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <algorithms/test_for_each.hpp>

#include <boost/geometry/geometries/geometries.hpp>



template <typename P>
void test_all()
{
    test_geometry<P>
        (
            "POINT(1 1)"

            // per point
            , 1
            , "POINT(101 1)"
            , "POINT(101 100)"
            // per segment
            , ""
            , 0
            , "POINT(1 1)"
        );
    test_geometry<bg::model::linestring<P> >
        (
            "LINESTRING(1 1,2 2)"

            , 3
            , "LINESTRING(101 1,102 2)"
            , "LINESTRING(101 100,102 200)"

            , "((1, 1), (2, 2))"
            , std::sqrt(2.0)
            , "LINESTRING(10 1,2 2)"
        );
    test_geometry<bg::model::ring<P> >
        (
            "POLYGON((1 1,1 4,4 4,4 1,1 1))"

            , 11
            , "POLYGON((101 1,101 4,104 4,104 1,101 1))"
            , "POLYGON((101 100,101 400,104 400,104 100,101 100))"

            , "((1, 1), (1, 4)) ((1, 4), (4, 4)) ((4, 4), (4, 1)) ((4, 1), (1, 1))"
            , 4 * 3.0
            , "POLYGON((10 1,10 4,4 4,4 1,1 1))"
        );
    test_geometry<bg::model::polygon<P> >
        (
            "POLYGON((1 1,1 4,4 4,4 1,1 1),(2 2,3 2,3 3,2 3,2 2))"

            , 23
            , "POLYGON((101 1,101 4,104 4,104 1,101 1),(102 2,103 2,103 3,102 3,102 2))"
            , "POLYGON((101 100,101 400,104 400,104 100,101 100),(102 200,103 200,103 300,102 300,102 200))"

            , "((1, 1), (1, 4)) ((1, 4), (4, 4)) ((4, 4), (4, 1)) ((4, 1), (1, 1)) "
              "((2, 2), (3, 2)) ((3, 2), (3, 3)) ((3, 3), (2, 3)) ((2, 3), (2, 2))"
            , 4 * 3.0 + 4 * 1.0
            , "POLYGON((10 1,10 4,4 4,4 1,1 1),(2 2,3 2,3 3,2 3,2 2))"
        );
}

int test_main(int, char* [])
{
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();

#if defined(HAVE_TTMATH)
    test_all<bg::model::point<ttmath_big, 2, bg::cs::cartesian> >();
#endif

    return 0;
}
