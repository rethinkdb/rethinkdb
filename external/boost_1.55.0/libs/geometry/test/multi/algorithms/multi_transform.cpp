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

#include <iostream>
#include <sstream>

#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/transform.hpp>
#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include <boost/geometry/multi/algorithms/transform.hpp>

#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>


// This test is a little different from transform.cpp test.
// This test explicitly tests all geometries, including multi*
// while the transform.cpp tests various strategies.

template <typename Geometry>
void test_transform(std::string const& wkt, std::string const& expected)
{
    typedef typename bg::coordinate_type<Geometry>::type coordinate_type;
    const std::size_t dim = bg::dimension<Geometry>::value;

    Geometry geometry_in, geometry_out;
    bg::read_wkt(wkt, geometry_in);
    bg::transform(geometry_in, geometry_out,
        bg::strategy::transform::scale_transformer<coordinate_type, dim, dim>(2, 2));
    std::ostringstream detected;
    detected << bg::wkt(geometry_out);
    BOOST_CHECK_EQUAL(detected.str(), expected);
}


template <typename T>
void test_all()
{
    typedef bg::model::d2::point_xy<T> P;

    test_transform<P>(
            "POINT(1 1)",
            "POINT(2 2)");
    test_transform<bg::model::linestring<P> >(
            "LINESTRING(1 1,2 2)",
            "LINESTRING(2 2,4 4)");
    test_transform<bg::model::segment<P> >(
            "LINESTRING(1 1,2 2)",
            "LINESTRING(2 2,4 4)");
    test_transform<bg::model::ring<P> >(
            "POLYGON((0 0,0 1,1 0,0 0))",
            "POLYGON((0 0,0 2,2 0,0 0))");
    test_transform<bg::model::polygon<P> >(
            "POLYGON((0 0,0 1,1 0,0 0))",
            "POLYGON((0 0,0 2,2 0,0 0))");
    test_transform<bg::model::box<P> >(
            "POLYGON((0 0,0 1,1 1,1 0,0 0))",
            "POLYGON((0 0,0 2,2 2,2 0,0 0))");
    test_transform<bg::model::multi_point<P> >(
            "MULTIPOINT((1 1),(2 2))",
            "MULTIPOINT((2 2),(4 4))");
    test_transform<bg::model::multi_linestring<bg::model::linestring<P> > >(
            "MULTILINESTRING((1 1,2 2))",
            "MULTILINESTRING((2 2,4 4))");
    test_transform<bg::model::multi_polygon<bg::model::polygon<P> > >(
            "MULTIPOLYGON(((0 0,0 1,1 0,0 0)))",
            "MULTIPOLYGON(((0 0,0 2,2 0,0 0)))");
}


int test_main(int, char* [])
{
    test_all<double>();

#ifdef HAVE_TTMATH
    test_all<ttmath_big>();
#endif
    return 0;
}
