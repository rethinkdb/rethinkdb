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

#include <iterator>


#include <algorithms/test_simplify.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include <test_geometries/wrapped_boost_array.hpp>
#include <test_common/test_point.hpp>


template <typename P>
void test_all()
{
    test_geometry<bg::model::linestring<P> >(
        "LINESTRING(0 0,5 5,10 10)",
        "LINESTRING(0 0,10 10)", 1.0);

    test_geometry<bg::model::linestring<P> >(
        "LINESTRING(0 0, 5 5, 6 5, 10 10)",
        "LINESTRING(0 0,10 10)", 1.0);

    test_geometry<bg::model::linestring<P> >(
        "LINESTRING(0 0,5 5,7 5,10 10)",
        "LINESTRING(0 0,5 5,7 5,10 10)", 1.0);

    /* TODO fix this
    test_geometry<test::wrapped_boost_array<P, 10> >(
        "LINESTRING(0 0,5 5,7 5,10 10)",
        "LINESTRING(0 0,5 5,7 5,10 10)", 1.0);
    */

    test_geometry<bg::model::ring<P> >(
        "POLYGON((4 0,8 2,8 7,4 9,0 7,0 2,2 1,4 0))",
        "POLYGON((4 0,8 2,8 7,4 9,0 7,0 2,4 0))", 1.0);

    test_geometry<bg::model::polygon<P> >(
        "POLYGON((4 0,8 2,8 7,4 9,0 7,0 2,2 1,4 0))",
        "POLYGON((4 0,8 2,8 7,4 9,0 7,0 2,4 0))", 1.0);

    test_geometry<bg::model::polygon<P> >(
        "POLYGON((4 0,8 2,8 7,4 9,0 7,0 2,2 1,4 0),(7 3,7 6,1 6,1 3,4 3,7 3))",
        "POLYGON((4 0,8 2,8 7,4 9,0 7,0 2,4 0),(7 3,7 6,1 6,1 3,7 3))", 1.0);

/*

Above can be checked in PostGIS by:

select astext(ST_Simplify(geomfromtext('LINESTRING(0 0, 5 5, 10 10)'),1.0)) as simplified
union all select astext(ST_Simplify(geomfromtext('LINESTRING(0 0, 5 5, 6 5, 10 10)'),1.0))
union all select astext(ST_Simplify(geomfromtext('LINESTRING(0 0, 5 5, 7 5, 10 10)'),1.0))
union all select astext(ST_Simplify(geomfromtext('POLYGON((4 0, 8 2, 8 7, 4 9, 0 7, 0 2, 2 1, 4 0))'),1.0))
union all select astext(ST_Simplify(geomfromtext('POLYGON((4 0, 8 2, 8 7, 4 9, 0 7, 0 2, 2 1, 4 0),(7 3, 7 6, 1 6, 1 3, 4 3, 7 3))'),1.0))
*/

    // Just check compilation
    test_geometry<P>(
        "POINT(0 0)",
        "POINT(0 0)", 1.0);


    test_geometry<bg::model::ring<P> >(
        "POLYGON((4 0,8 2,8 7,4 9,0 7,0 2,2 1,4 0))",
        "POLYGON((4 0,8 2,8 7,4 9,0 7,0 2,4 0))", 1.0);
}


template <typename P>
void test_spherical()
{
    test_geometry<bg::model::linestring<P> >(
        "LINESTRING(4.1 52.1,4.2 52.2,4.3 52.3)",
        "LINESTRING(4.1 52.1,4.3 52.3)", 0.01);
}


int test_main(int, char* [])
{
    // Integer compiles, but simplify-process fails (due to distances)
    //test_all<bg::model::d2::point_xy<int> >();

    test_all<bg::model::d2::point_xy<float> >();
    test_all<bg::model::d2::point_xy<double> >();

    test_spherical<bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree> > >();

#if defined(HAVE_TTMATH)
    test_all<bg::model::d2::point_xy<ttmath_big> >();
    test_spherical<bg::model::point<ttmath_big, 2, bg::cs::spherical_equatorial<bg::degree> > >();
#endif

    return 0;
}
