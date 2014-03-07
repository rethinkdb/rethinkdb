// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <algorithms/test_equals.hpp>

#include <boost/geometry/multi/algorithms/area.hpp>
#include <boost/geometry/multi/algorithms/equals.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/geometry/multi/io/wkt/read.hpp>



template <typename P>
void test_all()
{
    std::string case1 = "MULTIPOLYGON(((0 0,0 7,4 2,2 0,0 0)))";
    std::string case1_p     = "POLYGON((0 0,0 7,4 2,2 0,0 0))";

    typedef bg::model::polygon<P> polygon;
    typedef bg::model::multi_polygon<polygon> mp;
    test_geometry<mp, mp>("c1", case1, case1, true);

    test_geometry<mp, mp>("c2",
            "MULTIPOLYGON(((0 0,0 7.01,4 2,2 0,0 0)))",
            case1, false);

    // Different order == equal
    test_geometry<mp, mp>("c3",
            "MULTIPOLYGON(((0 0,0 7,4 2,2 0,0 0)),((10 10,10 12,12 10,10 10)))",
            "MULTIPOLYGON(((10 10,10 12,12 10,10 10)),((0 0,0 7,4 2,2 0,0 0)))",
            true);

    // check different types
    test_geometry<polygon, mp>("c1_p_mp", case1_p, case1, true);
    test_geometry<mp, polygon>("c1_mp_p", case1, case1_p, true);

}

int test_main( int , char* [] )
{
    test_all<bg::model::d2::point_xy<double> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
