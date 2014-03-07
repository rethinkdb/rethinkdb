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


#include <geometry_test_common.hpp>

#include <boost/geometry/strategies/cartesian/distance_projected_point.hpp>
#include <boost/geometry/strategies/concepts/distance_concept.hpp>

#include <boost/geometry/io/wkt/read.hpp>


#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <test_common/test_point.hpp>

#ifdef HAVE_TTMATH
#  include <boost/geometry/extensions/contrib/ttmath_stub.hpp>
#endif

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename P, typename PS, typename CalculationType>
void test_services()
{
    PS p1, p2;
    bg::assign_values(p1, 0, 0);
    bg::assign_values(p2, 0, 4);

    P p;
    bg::assign_values(p, 2, 0);

    CalculationType const sqr_expected = 4;
    CalculationType const expected = 2;


    namespace bgsd = bg::strategy::distance;
    namespace services = bg::strategy::distance::services;
    // 1: normal, calculate distance:

    typedef bgsd::projected_point<CalculationType> strategy_type;

    BOOST_CONCEPT_ASSERT( (bg::concept::PointSegmentDistanceStrategy<strategy_type, P, PS>) );

    typedef typename services::return_type<strategy_type, P, PS>::type return_type;

    strategy_type strategy;
    return_type result = strategy.apply(p, p1, p2);
    BOOST_CHECK_CLOSE(result, return_type(expected), 0.001);

    // 2: the strategy should return the same result if we reverse parameters
    result = strategy.apply(p, p1, p2);
    BOOST_CHECK_CLOSE(result, return_type(expected), 0.001);


    // 3: "comparable" to construct a "comparable strategy" for P1/P2
    //    a "comparable strategy" is a strategy which does not calculate the exact distance, but
    //    which returns results which can be mutually compared (e.g. avoid sqrt)

    // 3a: "comparable_type"
    typedef typename services::comparable_type<strategy_type>::type comparable_type;

    // 3b: "get_comparable"
    comparable_type comparable = bgsd::services::get_comparable<strategy_type>::apply(strategy);

    return_type c_result = comparable.apply(p, p1, p2);
    BOOST_CHECK_CLOSE(c_result, return_type(sqr_expected), 0.001);
}


template <typename P1, typename P2, typename T>
void test_all_2d(std::string const& wkt_p,
                 std::string const& wkt_sp1,
                 std::string const& wkt_sp2,
                 T expected_distance)
{
    P1 p;
    P2 sp1, sp2;
    bg::read_wkt(wkt_p, p);
    bg::read_wkt(wkt_sp1, sp1);
    bg::read_wkt(wkt_sp2, sp2);

    {
        typedef bg::strategy::distance::projected_point<> strategy_type;

        BOOST_CONCEPT_ASSERT
            (
                (bg::concept::PointSegmentDistanceStrategy<strategy_type, P1, P2>)
            );

        strategy_type strategy;
        typedef typename bg::strategy::distance::services::return_type<strategy_type, P1, P2>::type return_type;
        return_type d = strategy.apply(p, sp1, sp2);
        BOOST_CHECK_CLOSE(d, expected_distance, 0.001);
    }

    // Test combination with the comparable strategy
    {
        typedef bg::strategy::distance::projected_point
            <
                void,
                bg::strategy::distance::comparable::pythagoras<>
            > strategy_type;
        strategy_type strategy;
        typedef typename bg::strategy::distance::services::return_type<strategy_type, P1, P2>::type return_type;
        return_type d = strategy.apply(p, sp1, sp2);
        T expected_squared_distance = expected_distance * expected_distance;
        BOOST_CHECK_CLOSE(d, expected_squared_distance, 0.01);
    }

}

template <typename P1, typename P2>
void test_all_2d()
{
    test_all_2d<P1, P2>("POINT(1 1)", "POINT(0 0)", "POINT(2 3)", 0.27735203958327);
    test_all_2d<P1, P2>("POINT(2 2)", "POINT(1 4)", "POINT(4 1)", 0.5 * sqrt(2.0));
    test_all_2d<P1, P2>("POINT(6 1)", "POINT(1 4)", "POINT(4 1)", 2.0);
    test_all_2d<P1, P2>("POINT(1 6)", "POINT(1 4)", "POINT(4 1)", 2.0);
}

template <typename P>
void test_all_2d()
{
    //test_all_2d<P, int[2]>();
    //test_all_2d<P, float[2]>();
    //test_all_2d<P, double[2]>();
    //test_all_2d<P, test::test_point>();
    test_all_2d<P, bg::model::point<int, 2, bg::cs::cartesian> >();
    test_all_2d<P, bg::model::point<float, 2, bg::cs::cartesian> >();
    test_all_2d<P, bg::model::point<double, 2, bg::cs::cartesian> >();
    test_all_2d<P, bg::model::point<long double, 2, bg::cs::cartesian> >();
}

int test_main(int, char* [])
{
    test_all_2d<int[2]>();
    test_all_2d<float[2]>();
    test_all_2d<double[2]>();
    //test_all_2d<test::test_point>();

    test_all_2d<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_all_2d<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_all_2d<bg::model::point<double, 2, bg::cs::cartesian> >();

    test_services
        <
            bg::model::point<double, 2, bg::cs::cartesian>,
            bg::model::point<float, 2, bg::cs::cartesian>,
            long double
        >();


#if defined(HAVE_TTMATH)
    test_all_2d
        <
            bg::model::point<ttmath_big, 2, bg::cs::cartesian>,
            bg::model::point<ttmath_big, 2, bg::cs::cartesian>
        >();
#endif

    return 0;
}
