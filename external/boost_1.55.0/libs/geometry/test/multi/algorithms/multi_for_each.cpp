// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/for_each.hpp>
#include <boost/geometry/multi/algorithms/for_each.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>

#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <algorithms/test_for_each.hpp>


template <typename P>
void test_all()
{
    test_geometry<bg::model::multi_point<P> >
        (
            "MULTIPOINT((1 1))"

            // per point
            , 1
            , "MULTIPOINT((101 1))"
            , "MULTIPOINT((101 100))"
            // per segment
            , ""
            , 0
            , "MULTIPOINT((1 1))"
        );

    test_geometry<bg::model::multi_linestring<bg::model::linestring<P> > >
        (
        "MULTILINESTRING((1 1,2 2))"

            , 3
            , "MULTILINESTRING((101 1,102 2))"
            , "MULTILINESTRING((101 100,102 200))"

            , "((1, 1), (2, 2))"
            , std::sqrt(2.0)
            , "MULTILINESTRING((10 1,2 2))"
        );

    typedef bg::model::multi_polygon<bg::model::polygon<P> > mp;
    test_geometry<mp>
        (
            "MULTIPOLYGON(((1 1,1 4,4 4,4 1,1 1)))"

            , 11
            , "MULTIPOLYGON(((101 1,101 4,104 4,104 1,101 1)))"
            , "MULTIPOLYGON(((101 100,101 400,104 400,104 100,101 100)))"

            , "((1, 1), (1, 4)) ((1, 4), (4, 4)) ((4, 4), (4, 1)) ((4, 1), (1, 1))"
            , 4 * 3.0
            , "MULTIPOLYGON(((10 1,10 4,4 4,4 1,1 1)))"
        );
}

int test_main( int , char* [] )
{
    test_all<bg::model::d2::point_xy<double> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
