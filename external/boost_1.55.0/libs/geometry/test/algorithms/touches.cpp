// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithms/test_touches.hpp>


#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>



template <typename P>
void test_all()
{
    typedef bg::model::polygon<P> polygon;

    // Just a normal polygon
    test_self_touches<polygon>("POLYGON((0 0,0 4,1.5 2.5,2.5 1.5,4 0,0 0))", false);

    // Self intersecting
    test_self_touches<polygon>("POLYGON((1 2,1 1,2 1,2 2.25,3 2.25,3 0,0 0,0 3,3 3,2.75 2,1 2))", false);

    // Self touching at a point
    test_self_touches<polygon>("POLYGON((0 0,0 3,2 3,2 2,1 2,1 1,2 1,2 2,3 2,3 0,0 0))", true);

    // Self touching at a segment
    test_self_touches<polygon>("POLYGON((0 0,0 3,2 3,2 2,1 2,1 1,2 1,2 2.5,3 2.5,3 0,0 0))", true);

    // Touching at corner
    test_touches<polygon, polygon>
        (
            "POLYGON((0 0,0 100,100 100,100 0,0 0))", 
            "POLYGON((100 100,100 200,200 200, 200 100,100 100))",
            true
        );

    // Intersecting at corner
    test_touches<polygon, polygon>
        (
            "POLYGON((0 0,0 100,101 101,100 0,0 0))", 
            "POLYGON((100 100,100 200,200 200, 200 100,100 100))",
            false
        );

    // Touching at side (interior of a segment)
    test_touches<polygon, polygon>
        (
            "POLYGON((0 0,0 100,100 100,100 0,0 0))", 
            "POLYGON((200 0,100 50,200 100,200 0))",
            true
        );

    // Touching at side (partly collinear)
    test_touches<polygon, polygon>
        (
            "POLYGON((0 0,0 100,100 100,100 0,0 0))", 
            "POLYGON((200 20,100 20,100 80,200 80,200 20))",
            true
        );

    // Completely equal
    test_touches<polygon, polygon>
        (
            "POLYGON((0 0,0 100,100 100,100 0,0 0))", 
            "POLYGON((0 0,0 100,100 100,100 0,0 0))",
            false
        );

    // Spatially equal
    test_touches<polygon, polygon>
        (
            "POLYGON((0 0,0 100,100 100,100 0,0 0))", 
            "POLYGON((0 0,0 100,100 100,100 80,100 20,100 0,0 0))",
            false
        );

    // Spatially equal (without equal segments)
    test_touches<polygon, polygon>
        (
            "POLYGON((0 0,0 100,100 100,100 0,0 0))", 
            "POLYGON((0 0,0 50,0 100,50 100,100 100,100 50,100 0,50 0,0 0))",
            false
        );
    

    // Touching at side (opposite equal)
    test_touches<polygon, polygon>
        (
            "POLYGON((0 0,0 100,100 100,100 0,0 0))", 
            "POLYGON((200 0,100 0,100 100,200 100,200 0))",
            true
        );

    // Touching at side (opposite equal - but with real "equal" turning point)
    test_touches<polygon, polygon>
        (
            "POLYGON((0 0,0 100,100 100,100 80,100 20,100 0,0 0))", 
            "POLYGON((200 0,100 0,100 20,100 80,100 100,200 100,200 0))",
            true
        );
    // First partly collinear to side, than overlapping
    test_touches<polygon, polygon>
        (
            "POLYGON((0 0,0 100,100 100,100 0,0 0))", 
            "POLYGON((200 20,100 20,100 50,50 50,50 80,100 80,200 80,200 20))",
            false
        );

    // Touching interior (= no touch)
    test_touches<polygon, polygon>
        (
            "POLYGON((0 0,0 100,100 100,100 0,0 0))", 
            "POLYGON((20 20,20 80,100 50,20 20))",
            false
        );

    // Fitting in hole
    test_touches<polygon, polygon>
        (
            "POLYGON((0 0,0 100,100 100,100 0,0 0),(40 40,60 40,60 60,40 60,40 40))", 
            "POLYGON((40 40,40 60,60 60,60 40,40 40))",
            true
        );

}




int test_main( int , char* [] )
{
    test_all<bg::model::d2::point_xy<double> >();


#if defined(HAVE_TTMATH)
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}

/*
with viewy as
(
select geometry::STGeomFromText('POLYGON((0 0,0 100,100 100,100 0,0 0))',0) as p
	, geometry::STGeomFromText('POLYGON((200 0,100 50,200 100,200 0))',0) as q
)
-- select p from viewy union all select q from viewy
select p.STTouches(q) from viewy
*/