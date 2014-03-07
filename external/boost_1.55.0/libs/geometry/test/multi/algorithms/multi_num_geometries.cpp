// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/num_geometries.hpp>
#include <boost/geometry/multi/algorithms/num_geometries.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

template <typename Geometry>
void test_geometry(std::string const& wkt, int expected)
{
    Geometry geometry;
    bg::read_wkt(wkt, geometry);
    int detected = bg::num_geometries(geometry);
    BOOST_CHECK_MESSAGE(detected == expected,
        "num_geometries: " << wkt
        << " -> Expected: " << expected
        << " detected: " << detected);
}


template <typename Point>
void test_all()
{
    typedef bg::model::polygon<Point> poly;
    typedef bg::model::linestring<Point> ls;
    typedef bg::model::multi_point<Point> mpoint;
    typedef bg::model::multi_linestring<ls> mls;
    typedef bg::model::multi_polygon<poly> mpoly;

    test_geometry<Point>("POINT(0 0)", 1);
    test_geometry<ls>("LINESTRING(0 0,0 1)", 1);
    test_geometry<poly>("POLYGON((0 0,0 1,1 0,0 0))", 1);
    test_geometry<mpoint>("MULTIPOINT((0 0),(0 1),(1 0),(0 0))", 4);
    test_geometry<mls>("MULTILINESTRING((0 0,0 1),(1 0,0 0))", 2);
    test_geometry<mpoly>("MULTIPOLYGON(((0 0,0 1,1 0,0 0)),((10 0,10 1,11 0,10 0)))", 2);
}


int test_main( int , char* [] )
{
    test_all<bg::model::d2::point_xy<double> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
