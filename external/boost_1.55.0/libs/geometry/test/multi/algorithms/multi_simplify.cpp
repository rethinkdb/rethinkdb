// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <geometry_test_common.hpp>

#include <boost/geometry/multi/algorithms/simplify.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>

#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <boost/geometry/geometries/point_xy.hpp>

#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <algorithms/test_simplify.hpp>


template <typename P>
void test_all()
{
    test_geometry<bg::model::multi_point<P> >(
        "MULTIPOINT((0 0),(1 1))",
        "MULTIPOINT((0 0),(1 1))", 1.0);

    test_geometry<bg::model::multi_linestring<bg::model::linestring<P> > >(
        "MULTILINESTRING((0 0,5 5,10 10))",
        "MULTILINESTRING((0 0,10 10))", 1.0);

    typedef bg::model::multi_polygon<bg::model::polygon<P> > mp;
    test_geometry<mp>(
        "MULTIPOLYGON(((4 0,8 2,8 7,4 9,0 7,0 2,2 1,4 0)))",
        "MULTIPOLYGON(((4 0,8 2,8 7,4 9,0 7,0 2,4 0)))", 1.0);
    test_geometry<mp>(
        "MULTIPOLYGON(((4 0,8 2,8 7,4 9,0 7,0 2,2 1,4 0),(7 3,7 6,1 6,1 3,4 3,7 3)))",
        "MULTIPOLYGON(((4 0,8 2,8 7,4 9,0 7,0 2,4 0),(7 3,7 6,1 6,1 3,7 3)))", 1.0);

    // Ticket 5954 https://svn.boost.org/trac/boost/ticket/5954
    test_geometry<mp>(
        "MULTIPOLYGON(((0.561648 1,1 1,1 0,0.468083 0,0.52758 0.00800554,0.599683 0.0280924,0.601611 0.265374,0.622693 0.316765,0.69507 0.357497,0.695623 0.429711,0.655111 0.502298,0.696467 0.543147,0.840712 0.593546,0.882583 0.66546,0.852357 0.748213,0.84264 0.789567,0.832667 0.841202,0.832667 0.841202,0.740538 0.873004,0.617349 0.905045,0.566576 0.977697,0.561648 1)),((0 0.801979,0.0308575 0.786234,0.0705513 0.631135,0.141616 0.527248,0.233985 0.505872,0.264777 0.526263,0.336631 0.505009,0.356603 0.422321,0.355803 0.350038,0.375252 0.205364,0.415206 0.0709182,0.45479 0,0 0,0 0,0 0.801979)))",
        "MULTIPOLYGON(((0.561648 1,1 1,1 0,0.468083 0,0.52758 0.00800554,0.599683 0.0280924,0.601611 0.265374,0.622693 0.316765,0.69507 0.357497,0.695623 0.429711,0.655111 0.502298,0.696467 0.543147,0.840712 0.593546,0.882583 0.66546,0.852357 0.748213,0.84264 0.789567,0.832667 0.841202,0.740538 0.873004,0.617349 0.905045,0.566576 0.977697,0.561648 1)),((0 0.801979,0.0308575 0.786234,0.0705513 0.631135,0.141616 0.527248,0.233985 0.505872,0.264777 0.526263,0.336631 0.505009,0.356603 0.422321,0.355803 0.350038,0.375252 0.205364,0.415206 0.0709182,0.45479 0,0 0,0 0.801979)))", 1.0 / 2048.0);
    test_geometry<mp>(
        "MULTIPOLYGON(((1149.69 2047,2047 2047,2047 0,958.166 0,1079.96 16.3873,1227.55 57.5051,1231.5 543.221,1274.65 648.418,1422.81 731.796,1423.94 879.618,1341.01 1028.2,1425.67 1111.82,1720.94 1214.99,1806.65 1362.2,1744.77 1531.59,1724.88 1616.24,1704.47 1721.94,1704.47 1721.94,1515.88 1787.04,1263.71 1852.63,1159.78 2001.35,1149.69 2047)),((0 1641.65,63.1653 1609.42,144.419 1291.93,289.888 1079.28,478.967 1035.52,541.999 1077.26,689.084 1033.75,729.966 864.491,728.329 716.528,768.141 420.38,849.927 145.17,930.955 0,0 0,0 0,0 1641.65)))",
        "MULTIPOLYGON(((1149.69 2047,2047 2047,2047 0,958.166 0,1079.96 16.3873,1227.55 57.5051,1231.5 543.221,1274.65 648.418,1422.81 731.796,1423.94 879.618,1341.01 1028.2,1425.67 1111.82,1720.94 1214.99,1806.65 1362.2,1744.77 1531.59,1724.88 1616.24,1704.47 1721.94,1515.88 1787.04,1263.71 1852.63,1159.78 2001.35,1149.69 2047)),((0 1641.65,63.1653 1609.42,144.419 1291.93,289.888 1079.28,478.967 1035.52,541.999 1077.26,689.084 1033.75,729.966 864.491,728.329 716.528,768.141 420.38,849.927 145.17,930.955 0,0 0,0 1641.65)))", 1.0);
    // End ticket 5954
}

int test_main( int , char* [] )
{
    test_all<bg::model::d2::point_xy<double> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
