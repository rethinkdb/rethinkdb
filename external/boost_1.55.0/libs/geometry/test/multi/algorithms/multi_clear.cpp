// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/multi/algorithms/clear.hpp>
#include <boost/geometry/multi/algorithms/num_points.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/variant/variant.hpp>


template <typename Geometry>
void test_geometry(std::string const& wkt, unsigned int expected)
{
    Geometry geometry;
    bg::read_wkt(wkt, geometry);
    boost::variant<Geometry> variant_geometry(geometry);

    bg::clear(geometry);
    BOOST_CHECK_EQUAL(bg::num_points(geometry), expected);

    bg::clear(variant_geometry);
    BOOST_CHECK_EQUAL(bg::num_points(variant_geometry), expected);
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
    test_geometry<ls>("LINESTRING(0 0,0 1)", 0);
    test_geometry<poly>("POLYGON((0 0,0 1,1 0,0 0))", 0);
    test_geometry<mpoint>("MULTIPOINT((0 0),(0 1),(1 0),(0 0))", 0);
    test_geometry<mls>("MULTILINESTRING((0 0,0 1),(1 0,0 0))", 0);
    test_geometry<mpoly>("MULTIPOLYGON(((0 0,0 1,1 0,0 0)),((10 0,10 1,11 0,10 0)))", 0);
}


int test_main( int , char* [] )
{
    test_all<bg::model::d2::point_xy<double> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
