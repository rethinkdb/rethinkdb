// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <algorithms/test_touches.hpp>

#include <boost/geometry/multi/algorithms/area.hpp>
#include <boost/geometry/multi/algorithms/num_geometries.hpp>
#include <boost/geometry/multi/algorithms/within.hpp>
#include <boost/geometry/multi/algorithms/detail/sections/range_by_section.hpp>
#include <boost/geometry/multi/algorithms/detail/sections/sectionalize.hpp>
#include <boost/geometry/multi/algorithms/detail/for_each_range.hpp>
#include <boost/geometry/multi/core/closure.hpp>
#include <boost/geometry/multi/core/geometry_id.hpp>
#include <boost/geometry/multi/core/ring_type.hpp>
#include <boost/geometry/multi/views/detail/range_type.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/geometry/multi/io/wkt/read.hpp>



template <typename P>
void test_all()
{
    typedef bg::model::polygon<P> polygon;
    typedef bg::model::multi_polygon<polygon> mp;

    test_self_touches<mp>("MULTIPOLYGON(((0 0,0 100,100 100,100 0,0 0)))",
            false);

    // Exactly equal
    test_touches<mp, mp>("MULTIPOLYGON(((0 0,0 100,100 100,100 0,0 0)))",
            "MULTIPOLYGON(((0 0,0 100,100 100,100 0,0 0)))",
            false);

    // Spatially equal
    test_touches<mp, mp>("MULTIPOLYGON(((0 0,0 100,100 100,100 0,0 0)))",
            "MULTIPOLYGON(((0 0,0 100,100 100,100 80,100 20,100 0,0 0)))",
            false);

    // One exactly equal, another pair touching
    test_touches<mp, mp>("MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((20 0,20 10,30 10,30 0,20 0)))",
            "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((30 10,30 20,40 20,40 10,30 10)))",
            false);

    // One spatially equal (without equal segments), another pair touching
    test_touches<mp, mp>("MULTIPOLYGON(((0 0,0 5,0 10,5 10,10 10,10 5,10 0,5 0,0 0)),((20 0,20 10,30 10,30 0,20 0)))",
            "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((30 10,30 20,40 20,40 10,30 10)))",
            false);

    // Alternate touches
    test_touches<mp, mp>("MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((20 0,20 10,30 10,30 0,20 0)))",
            "MULTIPOLYGON(((10 10,10 20,20 20,20 10,10 10)),((30 10,30 20,40 20,40 10,30 10)))",
            true);

    // Touch plus inside
    // TODO fix this
    test_touches<mp, mp>("MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((20 0,20 10,30 10,30 0,20 0)))",
            "MULTIPOLYGON(((10 10,10 20,20 20,20 10,10 10)),((22 2,28 2,28 8,22 8,22 2)))",
            false);
}

int test_main( int , char* [] )
{
    test_all<bg::model::d2::point_xy<double> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
