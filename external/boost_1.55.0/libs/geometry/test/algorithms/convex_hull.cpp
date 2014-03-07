// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <cstddef>
#include <string>

#include <algorithms/test_convex_hull.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>


template <typename P>
void test_all()
{
    // from sample linestring
    test_geometry<bg::model::linestring<P> >(
        "linestring(1.1 1.1, 2.5 2.1, 3.1 3.1, 4.9 1.1, 3.1 1.9)", 5, 4, 3.8);

    // rectangular, with concavity
    test_geometry<bg::model::polygon<P> >(
        "polygon((1 1, 1 4, 3 4, 3 3, 4 3, 4 4, 5 4, 5 1, 1 1))",
                9, 5, 12.0);

    // from sample polygon, with concavity
    test_geometry<bg::model::polygon<P> >(
        "polygon((2.0 1.3, 2.4 1.7, 2.8 1.8, 3.4 1.2, 3.7 1.6,3.4 2.0, 4.1 3.0"
        ", 5.3 2.6, 5.4 1.2, 4.9 0.8, 2.9 0.7,2.0 1.3))",
                12, 8, 5.245);

    test_geometry<bg::model::ring<P> >(
        "polygon((2.0 1.3, 2.4 1.7, 2.8 1.8, 3.4 1.2, 3.7 1.6,3.4 2.0, 4.1 3.0"
        ", 5.3 2.6, 5.4 1.2, 4.9 0.8, 2.9 0.7,2.0 1.3))",
                12, 8, 5.245);

    test_geometry<bg::model::box<P> >("box(0 0,2 2)", 4, 5, 4);
    
    test_empty_input<bg::model::linestring<P> >();
    test_empty_input<bg::model::ring<P> >();
    test_empty_input<bg::model::polygon<P> >();
}


int test_main(int, char* [])
{
    //test_all<bg::model::d2::point_xy<int> >();
    test_all<bg::model::d2::point_xy<float> >();
    test_all<bg::model::d2::point_xy<double> >();

#if defined(HAVE_TTMATH)
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
