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

#define TEST_ARRAY

#include <sstream>

#include <algorithms/test_distance.hpp>

#include <boost/mpl/if.hpp>
#include <boost/array.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

#include <test_common/test_point.hpp>
#include <test_geometries/custom_segment.hpp>
#include <test_geometries/wrapped_boost_array.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)

// Register boost array as a linestring
namespace boost { namespace geometry { namespace traits
{
template <typename Point, std::size_t PointCount>
struct tag< boost::array<Point, PointCount> >
{
    typedef linestring_tag type;
};

}}}

template <typename P>
void test_distance_point()
{
    namespace services = bg::strategy::distance::services;
    typedef typename bg::default_distance_result<P>::type return_type;

    // Basic, trivial test

    P p1;
    bg::set<0>(p1, 1);
    bg::set<1>(p1, 1);

    P p2;
    bg::set<0>(p2, 2);
    bg::set<1>(p2, 2);

    return_type d = bg::distance(p1, p2);
    BOOST_CHECK_CLOSE(d, return_type(1.4142135), 0.001);

    // Test specifying strategy manually
    typename services::default_strategy<bg::point_tag, P>::type strategy;

    d = bg::distance(p1, p2, strategy);
    BOOST_CHECK_CLOSE(d, return_type(1.4142135), 0.001);

    {
        // Test custom strategy
        BOOST_CONCEPT_ASSERT( (bg::concept::PointDistanceStrategy<taxicab_distance, P, P>) );

        typedef typename services::return_type<taxicab_distance, P, P>::type cab_return_type;
        BOOST_MPL_ASSERT((boost::is_same<cab_return_type, typename bg::coordinate_type<P>::type>));

        taxicab_distance tcd;
        cab_return_type d = bg::distance(p1, p2, tcd);

        BOOST_CHECK( bg::math::abs(d - cab_return_type(2)) <= cab_return_type(0.01) );
    }

    {
        // test comparability

        typedef typename services::default_strategy<bg::point_tag, P>::type strategy_type;
        typedef typename services::comparable_type<strategy_type>::type comparable_strategy_type;

        strategy_type strategy;
        comparable_strategy_type comparable_strategy = services::get_comparable<strategy_type>::apply(strategy);
        return_type comparable = services::result_from_distance<comparable_strategy_type, P, P>::apply(comparable_strategy, 3);

        BOOST_CHECK_CLOSE(comparable, return_type(9), 0.001);
    }
}

template <typename P>
void test_distance_segment()
{
    typedef typename bg::default_distance_result<P>::type return_type;
    typedef typename bg::coordinate_type<P>::type coordinate_type;

    P s1; bg::set<0>(s1, 1); bg::set<1>(s1, 1);
    P s2; bg::set<0>(s2, 4); bg::set<1>(s2, 4);

    // Check points left, right, projected-left, projected-right, on segment
    P p1; bg::set<0>(p1, 0); bg::set<1>(p1, 1);
    P p2; bg::set<0>(p2, 1); bg::set<1>(p2, 0);
    P p3; bg::set<0>(p3, 3); bg::set<1>(p3, 1);
    P p4; bg::set<0>(p4, 1); bg::set<1>(p4, 3);
    P p5; bg::set<0>(p5, 3); bg::set<1>(p5, 3);

    bg::model::referring_segment<P const> const seg(s1, s2);

    return_type d1 = bg::distance(p1, seg);
    return_type d2 = bg::distance(p2, seg);
    return_type d3 = bg::distance(p3, seg);
    return_type d4 = bg::distance(p4, seg);
    return_type d5 = bg::distance(p5, seg);

    BOOST_CHECK_CLOSE(d1, return_type(1), 0.001);
    BOOST_CHECK_CLOSE(d2, return_type(1), 0.001);
    BOOST_CHECK_CLOSE(d3, return_type(1.4142135), 0.001);
    BOOST_CHECK_CLOSE(d4, return_type(1.4142135), 0.001);
    BOOST_CHECK_CLOSE(d5, return_type(0), 0.001);

    // Reverse case: segment/point instead of point/segment
    return_type dr1 = bg::distance(seg, p1);
    return_type dr2 = bg::distance(seg, p2);

    BOOST_CHECK_CLOSE(dr1, d1, 0.001);
    BOOST_CHECK_CLOSE(dr2, d2, 0.001);

    // Test specifying strategy manually:
    // 1) point-point-distance
    typename bg::strategy::distance::services::default_strategy<bg::point_tag, P>::type pp_strategy;
    d1 = bg::distance(p1, seg, pp_strategy);
    BOOST_CHECK_CLOSE(d1, return_type(1), 0.001);

    // 2) point-segment-distance
    typename bg::strategy::distance::services::default_strategy<bg::segment_tag, P>::type ps_strategy;
    d1 = bg::distance(p1, seg, ps_strategy);
    BOOST_CHECK_CLOSE(d1, return_type(1), 0.001);

    // 3) custom point strategy
    taxicab_distance tcd;
    d1 = bg::distance(p1, seg, tcd);
    BOOST_CHECK_CLOSE(d1, return_type(1), 0.001);
}


template <typename P>
void test_distance_array_as_linestring()
{
    typedef typename bg::default_distance_result<P>::type return_type;

    // Normal array does not have
    boost::array<P, 2> points;
    bg::set<0>(points[0], 1);
    bg::set<1>(points[0], 1);
    bg::set<0>(points[1], 3);
    bg::set<1>(points[1], 3);

    P p;
    bg::set<0>(p, 2);
    bg::set<1>(p, 1);

    return_type d = bg::distance(p, points);
    BOOST_CHECK_CLOSE(d, return_type(0.70710678), 0.001);

    bg::set<0>(p, 5); bg::set<1>(p, 5);
    d = bg::distance(p, points);
    BOOST_CHECK_CLOSE(d, return_type(2.828427), 0.001);
}




template <typename P>
void test_all()
{
    test_distance_point<P>();
    test_distance_segment<P>();
    test_distance_array_as_linestring<P>();

    test_geometry<P, bg::model::segment<P> >("POINT(1 3)", "LINESTRING(1 1,4 4)", sqrt(2.0));
    test_geometry<P, bg::model::segment<P> >("POINT(3 1)", "LINESTRING(1 1,4 4)", sqrt(2.0));

    test_geometry<P, P>("POINT(1 1)", "POINT(2 2)", sqrt(2.0));
    test_geometry<P, P>("POINT(0 0)", "POINT(0 3)", 3.0);
    test_geometry<P, P>("POINT(0 0)", "POINT(4 0)", 4.0);
    test_geometry<P, P>("POINT(0 3)", "POINT(4 0)", 5.0);
    test_geometry<P, bg::model::linestring<P> >("POINT(1 3)", "LINESTRING(1 1,4 4)", sqrt(2.0));
    test_geometry<P, bg::model::linestring<P> >("POINT(3 1)", "LINESTRING(1 1,4 4)", sqrt(2.0));
    test_geometry<P, bg::model::linestring<P> >("POINT(50 50)", "LINESTRING(50 40, 40 50)", sqrt(50.0));
    test_geometry<P, bg::model::linestring<P> >("POINT(50 50)", "LINESTRING(50 40, 40 50, 0 90)", sqrt(50.0));
    test_geometry<bg::model::linestring<P>, P>("LINESTRING(1 1,4 4)", "POINT(1 3)", sqrt(2.0));
    test_geometry<bg::model::linestring<P>, P>("LINESTRING(50 40, 40 50)", "POINT(50 50)", sqrt(50.0));
    test_geometry<bg::model::linestring<P>, P>("LINESTRING(50 40, 40 50, 0 90)", "POINT(50 50)", sqrt(50.0));

    // Rings
    test_geometry<P, bg::model::ring<P> >("POINT(1 3)", "POLYGON((1 1,4 4,5 0,1 1))", sqrt(2.0));
    test_geometry<P, bg::model::ring<P> >("POINT(3 1)", "POLYGON((1 1,4 4,5 0,1 1))", 0.0);
    // other way round
    test_geometry<bg::model::ring<P>, P>("POLYGON((1 1,4 4,5 0,1 1))", "POINT(3 1)", 0.0);
    // open ring
    test_geometry<P, bg::model::ring<P, true, false> >("POINT(1 3)", "POLYGON((4 4,5 0,1 1))", sqrt(2.0));

    // Polygons
    test_geometry<P, bg::model::polygon<P> >("POINT(1 3)", "POLYGON((1 1,4 4,5 0,1 1))", sqrt(2.0));
    test_geometry<P, bg::model::polygon<P> >("POINT(3 1)", "POLYGON((1 1,4 4,5 0,1 1))", 0.0);
    // other way round
    test_geometry<bg::model::polygon<P>, P>("POLYGON((1 1,4 4,5 0,1 1))", "POINT(3 1)", 0.0);
    // open polygon
    test_geometry<P, bg::model::polygon<P, true, false> >("POINT(1 3)", "POLYGON((4 4,5 0,1 1))", sqrt(2.0));

    // Polygons with holes
    std::string donut = "POLYGON ((0 0,1 9,8 1,0 0),(1 1,4 1,1 4,1 1))";
    test_geometry<P, bg::model::polygon<P> >("POINT(2 2)", donut, 0.5 * sqrt(2.0));
    test_geometry<P, bg::model::polygon<P> >("POINT(3 3)", donut, 0.0);
    // other way round
    test_geometry<bg::model::polygon<P>, P>(donut, "POINT(2 2)", 0.5 * sqrt(2.0));
    // open
    test_geometry<P, bg::model::polygon<P, true, false> >("POINT(2 2)", "POLYGON ((0 0,1 9,8 1),(1 1,4 1,1 4))", 0.5 * sqrt(2.0));

    // Should (currently) give compiler assertion
    // test_geometry<bg::model::polygon<P>, bg::model::polygon<P> >(donut, donut, 0.5 * sqrt(2.0));

    // DOES NOT COMPILE - cannot do read_wkt (because boost::array is not variably sized)
    // test_geometry<P, boost::array<P, 2> >("POINT(3 1)", "LINESTRING(1 1,4 4)", sqrt(2.0));

    test_geometry<P, test::wrapped_boost_array<P, 2> >("POINT(3 1)", "LINESTRING(1 1,4 4)", sqrt(2.0));

}

template <typename P>
void test_empty_input()
{
    P p;
    bg::model::linestring<P> line_empty;
    bg::model::polygon<P> poly_empty;
    bg::model::ring<P> ring_empty;

    test_empty_input(p, line_empty);
    test_empty_input(p, poly_empty);
    test_empty_input(p, ring_empty);
}

void test_large_integers()
{
    typedef bg::model::point<int, 2, bg::cs::cartesian> int_point_type;
    typedef bg::model::point<double, 2, bg::cs::cartesian> double_point_type;

    // point-point
    {
        std::string const a = "POINT(2544000 528000)";
        std::string const b = "POINT(2768040 528000)";
        int_point_type ia, ib;
        double_point_type da, db;
        bg::read_wkt(a, ia);
        bg::read_wkt(b, ib);
        bg::read_wkt(a, da);
        bg::read_wkt(b, db);

        BOOST_AUTO(idist, bg::distance(ia, ib));
        BOOST_AUTO(ddist, bg::distance(da, db));

        BOOST_CHECK_MESSAGE(std::abs(idist - ddist) < 0.1, 
                "within<a double> different from within<an int>");
    }
    // Point-segment
    {
        std::string const a = "POINT(2600000 529000)";
        std::string const b = "LINESTRING(2544000 528000, 2768040 528000)";
        int_point_type ia;
        double_point_type da;
        bg::model::segment<int_point_type> ib;
        bg::model::segment<double_point_type> db;
        bg::read_wkt(a, ia);
        bg::read_wkt(b, ib);
        bg::read_wkt(a, da);
        bg::read_wkt(b, db);

        BOOST_AUTO(idist, bg::distance(ia, ib));
        BOOST_AUTO(ddist, bg::distance(da, db));

        BOOST_CHECK_MESSAGE(std::abs(idist - ddist) < 0.1, 
                "within<a double> different from within<an int>");
    }
}

int test_main(int, char* [])
{
#ifdef TEST_ARRAY
    //test_all<int[2]>();
    //test_all<float[2]>();
    //test_all<double[2]>();
    //test_all<test::test_point>(); // located here because of 3D
#endif

    test_large_integers();

    test_all<bg::model::d2::point_xy<int> >();
    test_all<boost::tuple<float, float> >();
    test_all<bg::model::d2::point_xy<float> >();
    test_all<bg::model::d2::point_xy<double> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    test_empty_input<bg::model::d2::point_xy<int> >();

    return 0;
}
