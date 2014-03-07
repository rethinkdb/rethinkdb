// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/correct.hpp>

#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/multi/algorithms/correct.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>

#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>


template <typename Geometry>
void test_geometry(std::string const& wkt, std::string const& expected)
{
    Geometry geometry;

    bg::read_wkt(wkt, geometry);
    bg::correct(geometry);

    std::ostringstream out;
    out << bg::wkt(geometry);

    BOOST_CHECK_EQUAL(out.str(), expected);
}

template <typename P>
void test_all()
{
    typedef bg::model::multi_polygon<bg::model::polygon<P> > cw_type;
    std::string cw_mp =
            "MULTIPOLYGON(((0 0,0 1,1 1,1 0,0 0)))";
    test_geometry<cw_type>(cw_mp, cw_mp);

    test_geometry<cw_type>("MULTIPOLYGON(((0 0,1 0,1 1,0 1,0 0)))",
        cw_mp);
}

int test_main( int , char* [] )
{
    test_all<bg::model::d2::point_xy<double> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
