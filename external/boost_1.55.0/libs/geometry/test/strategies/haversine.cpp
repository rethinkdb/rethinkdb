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

#include <boost/concept/requires.hpp>
#include <boost/concept_check.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>
#include <boost/geometry/strategies/concepts/distance_concept.hpp>


#include <boost/geometry/geometries/point.hpp>

#ifdef HAVE_TTMATH
#  include <boost/geometry/extensions/contrib/ttmath_stub.hpp>
#endif



double const average_earth_radius = 6372795.0;


template <typename Point, typename LatitudePolicy>
struct test_distance
{
    typedef bg::strategy::distance::haversine<double> haversine_type;
    typedef typename bg::strategy::distance::services::return_type<haversine_type, Point, Point>::type return_type;

    BOOST_CONCEPT_ASSERT
        (
            (bg::concept::PointDistanceStrategy<haversine_type, Point, Point>)
        );


    static void test(double lon1, double lat1, double lon2, double lat2,
                       double radius, double expected, double tolerance)
    {
        haversine_type strategy(radius);

        Point p1, p2;
        bg::assign_values(p1, lon1, LatitudePolicy::apply(lat1));
        bg::assign_values(p2, lon2, LatitudePolicy::apply(lat2));
        return_type d = strategy.apply(p1, p2);

        BOOST_CHECK_CLOSE(d, expected, tolerance);
    }
};

template <typename Point, typename LatitudePolicy>
void test_all()
{
    // earth to unit-sphere -> divide by earth circumference, then it is from 0-1,
    // then multiply with 2 PI, so effectively just divide by earth radius
    double e2u = 1.0 / average_earth_radius;

    // ~ Amsterdam/Paris, 467 kilometers
    double const a_p = 467.2704 * 1000.0;
    test_distance<Point, LatitudePolicy>::test(4, 52, 2, 48, average_earth_radius, a_p, 1.0);
    test_distance<Point, LatitudePolicy>::test(2, 48, 4, 52, average_earth_radius, a_p, 1.0);
    test_distance<Point, LatitudePolicy>::test(4, 52, 2, 48, 1.0, a_p * e2u, 0.001);

    // ~ Amsterdam/Barcelona
    double const a_b = 1232.9065 * 1000.0;
    test_distance<Point, LatitudePolicy>::test(4, 52, 2, 41, average_earth_radius, a_b, 1.0);
    test_distance<Point, LatitudePolicy>::test(2, 41, 4, 52, average_earth_radius, a_b, 1.0);
    test_distance<Point, LatitudePolicy>::test(4, 52, 2, 41, 1.0, a_b * e2u, 0.001);
}


template <typename P1, typename P2, typename CalculationType, typename LatitudePolicy>
void test_services()
{
    namespace bgsd = bg::strategy::distance;
    namespace services = bg::strategy::distance::services;

    {

        // Compile-check if there is a strategy for this type
        typedef typename services::default_strategy<bg::point_tag, P1, P2>::type haversine_strategy_type;
    }

    P1 p1;
    bg::assign_values(p1, 4, 52);

    P2 p2;
    bg::assign_values(p2, 2, 48);

    // ~ Amsterdam/Paris, 467 kilometers
    double const km = 1000.0;
    double const a_p = 467.2704 * km;
    double const expected = a_p;

    double const expected_lower = 460.0 * km;
    double const expected_higher = 470.0 * km;

    // 1: normal, calculate distance:

    typedef bgsd::haversine<double, CalculationType> strategy_type;
    typedef typename bgsd::services::return_type<strategy_type, P1, P2>::type return_type;

    strategy_type strategy(average_earth_radius);
    return_type result = strategy.apply(p1, p2);
    BOOST_CHECK_CLOSE(result, return_type(expected), 0.001);

    // 2: the strategy should return the same result if we reverse parameters
    result = strategy.apply(p2, p1);
    BOOST_CHECK_CLOSE(result, return_type(expected), 0.001);


    // 3: "comparable" to construct a "comparable strategy" for P1/P2
    //    a "comparable strategy" is a strategy which does not calculate the exact distance, but
    //    which returns results which can be mutually compared (e.g. avoid sqrt)

    // 3a: "comparable_type"
    typedef typename services::comparable_type<strategy_type>::type comparable_type;

    // 3b: "get_comparable"
    comparable_type comparable = bgsd::services::get_comparable<strategy_type>::apply(strategy);

    // Check vice versa:
    // First the result of the comparable strategy
    return_type c_result = comparable.apply(p1, p2);
    // Second the comparable result of the expected distance
    return_type c_expected = services::result_from_distance<comparable_type, P1, P2>::apply(comparable, expected);
    // And that one should be equa.
    BOOST_CHECK_CLOSE(c_result, return_type(c_expected), 0.001);

    // 4: the comparable_type should have a distance_strategy_constructor as well,
    //    knowing how to compare something with a fixed distance
    return_type c_dist_lower = services::result_from_distance<comparable_type, P1, P2>::apply(comparable, expected_lower);
    return_type c_dist_higher = services::result_from_distance<comparable_type, P1, P2>::apply(comparable, expected_higher);

    // If this is the case:
    BOOST_CHECK(c_dist_lower < c_result && c_result < c_dist_higher);

    // Calculate the Haversine by hand here:
    return_type c_check = return_type(2.0) * asin(sqrt(c_result)) * average_earth_radius;
    BOOST_CHECK_CLOSE(c_check, expected, 0.001);

    // This should also be the case
    return_type dist_lower = services::result_from_distance<strategy_type, P1, P2>::apply(strategy, expected_lower);
    return_type dist_higher = services::result_from_distance<strategy_type, P1, P2>::apply(strategy, expected_higher);
    BOOST_CHECK(dist_lower < result && result < dist_higher);
}

/****
template <typename P, typename Strategy>
void time_compare_s(int const n)
{
    boost::timer t;
    P p1, p2;
    bg::assign_values(p1, 1, 1);
    bg::assign_values(p2, 2, 2);
    Strategy strategy;
    typename Strategy::return_type s = 0;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            s += strategy.apply(p1, p2);
        }
    }
    std::cout << "s: " << s << " t: " << t.elapsed() << std::endl;
}

template <typename P>
void time_compare(int const n)
{
    time_compare_s<P, bg::strategy::distance::haversine<double> >(n);
    time_compare_s<P, bg::strategy::distance::comparable::haversine<double> >(n);
}

#include <time.h>
double time_sqrt(int n)
{
    clock_t start = clock();

    double v = 2.0;
    double s = 0.0;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            s += sqrt(v);
            v += 1.0e-10;
        }
    }
    clock_t end = clock();
    double diff = double(end - start) / CLOCKS_PER_SEC;

    std::cout << "Check: " << s << " Time: " << diff << std::endl;
    return diff;
}

double time_normal(int n)
{
    clock_t start = clock();

    double v = 2.0;
    double s = 0.0;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            s += v;
            v += 1.0e-10;
        }
    }
    clock_t end = clock();
    double diff = double(end - start) / CLOCKS_PER_SEC;

    std::cout << "Check: " << s << " Time: " << diff << std::endl;
    return diff;
}
***/

int test_main(int, char* [])
{
    test_all<bg::model::point<int, 2, bg::cs::spherical_equatorial<bg::degree> >, geographic_policy>();
    test_all<bg::model::point<float, 2, bg::cs::spherical_equatorial<bg::degree> >, geographic_policy>();
    test_all<bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree> >, geographic_policy>();

    // NYI: haversine for mathematical spherical coordinate systems
    // test_all<bg::model::point<double, 2, bg::cs::spherical<bg::degree> >, mathematical_policy>();

    //double t1 = time_sqrt(20000);
    //double t2 = time_normal(20000);
    //std::cout << "Factor: " << (t1 / t2) << std::endl;
    //time_compare<bg::model::point<double, 2, bg::cs::spherical<bg::radian> > >(10000);

#if defined(HAVE_TTMATH)
    typedef ttmath::Big<1,4> tt;
    test_all<bg::model::point<tt, 2, bg::cs::spherical_equatorial<bg::degree> >, geographic_policy>();
#endif


    test_services
        <
            bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree> >,
            bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree> >,
            double, 
            geographic_policy 
        >();

    return 0;
}
