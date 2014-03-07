// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/algorithms/covered_by.hpp>

#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <boost/geometry/multi/core/point_order.hpp>
#include <boost/geometry/multi/algorithms/covered_by.hpp>

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>

#include <algorithms/test_covered_by.hpp>


template <typename P>
void test_all()
{
    typedef bg::model::multi_polygon<bg::model::polygon<P> > mp;

    // test multi-with-one-polygon (trivial case)
    test_geometry<P, mp>("POINT(1 1)", "MULTIPOLYGON(((0 0,0 2,2 2,2 0,0 0)))", true);
    test_geometry<P, mp>("POINT(3 3)", "MULTIPOLYGON(((0 0,0 2,2 2,2 0,0 0)))", false);
    test_geometry<P, mp>("POINT(0 1)", "MULTIPOLYGON(((0 0,0 2,2 2,2 0,0 0)))", true);
    test_geometry<P, mp>("POINT(4 4)", "MULTIPOLYGON(((0 0,0 2,2 2,2 0,0 0)))", false);

    // test if it is in one of them
    std::string multi("MULTIPOLYGON("
        "((0 0,0 2,2 2,2 0,0 0))"
        "((3 3,3 6,6 6,6 3,3 3))"
        ")");
    test_geometry<P, mp>("POINT(4 4)", multi, true);
    test_geometry<P, mp>("POINT(1 1)", multi, true);
    test_geometry<P, mp>("POINT(0 1)", multi, true);
}

int test_main( int , char* [] )
{
    //test_all<bg::model::d2::point_xy<int> >();
    test_all<bg::model::d2::point_xy<double> >();

#if defined(HAVE_TTMATH)
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
