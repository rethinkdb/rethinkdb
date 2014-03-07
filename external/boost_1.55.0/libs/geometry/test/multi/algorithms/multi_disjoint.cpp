// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>


#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/multi/io/wkt/read.hpp>
#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/multi/algorithms/detail/sections/range_by_section.hpp>
#include <boost/geometry/multi/algorithms/detail/sections/sectionalize.hpp>
#include <boost/geometry/multi/algorithms/detail/point_on_border.hpp>
#include <boost/geometry/multi/algorithms/detail/for_each_range.hpp>
#include <boost/geometry/multi/algorithms/disjoint.hpp>
#include <boost/geometry/multi/algorithms/within.hpp>
#include <boost/geometry/multi/core/closure.hpp>
#include <boost/geometry/multi/core/geometry_id.hpp>
#include <boost/geometry/multi/core/ring_type.hpp>
#include <boost/geometry/multi/views/detail/range_type.hpp>


#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <test_common/test_point.hpp>

#include <algorithms/test_relate.hpp>


template <typename G1, typename G2>
void test_disjoint(std::string const& id,
            std::string const& wkt1,
            std::string const& wkt2, bool expected)
{
    G1 g1;
    bg::read_wkt(wkt1, g1);

    G2 g2;
    bg::read_wkt(wkt2, g2);

    bool detected = bg::disjoint(g1, g2);
    BOOST_CHECK_MESSAGE(detected == expected,
        "disjoint: " << id
        << " -> Expected: " << expected
        << " detected: " << detected);
}



template <typename P>
void test_all()
{
    typedef bg::model::polygon<P> polygon;
    typedef bg::model::multi_polygon<polygon> mp;

    test_disjoint<mp, mp>("", 
        "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
            "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
        false);

    // True disjoint
    test_disjoint<mp, mp>("", 
        "MULTIPOLYGON(((0 0,0 4,4 4,4 0,0 0)))",
            "MULTIPOLYGON(((6 6,6 10,10 10,10 6,6 6)))",
        true);

    // Touch -> not disjoint
    test_disjoint<mp, mp>("", 
        "MULTIPOLYGON(((0 0,0 5,5 5,5 0,0 0)))",
            "MULTIPOLYGON(((5 5,5 10,10 10,10 5,5 5)))",
        false);

    // Not disjoint but no IP's
    test_disjoint<mp, mp>("no_ips", 
        "MULTIPOLYGON(((2 2,2 8,8 8,8 2,2 2)),((20 0,20 10,30 10,30 0,20 0)))",
            "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((22 2,28 2,28 8,22 8,22 2)))",
        false);

    // Not disjoint and not inside each other (in first ring) but wrapped (in second rings)
    test_disjoint<mp, mp>("no_ips2", 
        "MULTIPOLYGON(((2 2,2 8,8 8,8 2,2 2)),((20 0,20 10,30 10,30 0,20 0)))",
            "MULTIPOLYGON(((2 12,2 18,8 18,8 12,2 12)),((22 2,28 2,28 8,22 8,22 2)))",
        false);

    test_disjoint<mp, mp>("no_ips2_rev", 
        "MULTIPOLYGON(((2 12,2 18,8 18,8 12,2 12)),((22 2,28 2,28 8,22 8,22 2)))",
        "MULTIPOLYGON(((2 2,2 8,8 8,8 2,2 2)),((20 0,20 10,30 10,30 0,20 0)))",
        false);


    test_disjoint<P, mp>("point_mp1", 
        "POINT(0 0)",
        "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
        false);

    test_disjoint<P, mp>("point_mp2", 
        "POINT(5 5)",
        "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
        false);

    test_disjoint<P, mp>("point_mp1", 
        "POINT(11 11)",
        "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
        true);

    std::string polygon_inside_hole("MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0), (2 2,8 2,8 8,2 8,2 2)),((4 4,4 6,6 6,6 4,4 4)))");
    test_disjoint<P, mp>("point_mp_pih1", 
        "POINT(5 5)",
        polygon_inside_hole,
        false);

    test_disjoint<P, mp>("point_mp_pih2", 
        "POINT(3 3)",
        polygon_inside_hole,
        true);

    test_disjoint<mp, P>("point_mp1rev", 
        "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
        "POINT(0 0)",
        false);

}

int test_main(int, char* [])
{
    //test_all<bg::model::d2::point_xy<float> >();
    test_all<bg::model::d2::point_xy<double> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}


/*
with viewy as
(
select geometry::STGeomFromText('MULTIPOLYGON(((2 2,2 8,8 8,8 2,2 2)),((20 0,20 10,30 10,30 0,20 0)))',0) as p
	, geometry::STGeomFromText('MULTIPOLYGON(((2 12,2 18,8 18,8 12,2 12)),((22 2,28 2,28 8,22 8,22 2)))',0) as q
)
 select p from viewy union all select q from viewy
-- select p.STDisjoint(q) from viewy
*/