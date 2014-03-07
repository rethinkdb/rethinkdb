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

#if defined(_MSC_VER)
#  pragma warning( disable : 4101 )
#endif

#include <boost/timer.hpp>

#include <boost/concept/requires.hpp>
#include <boost/concept_check.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/strategies/cartesian/distance_pythagoras.hpp>
#include <boost/geometry/strategies/concepts/distance_concept.hpp>


#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

#include <test_common/test_point.hpp>

#ifdef HAVE_TTMATH
#  include <boost/geometry/extensions/contrib/ttmath_stub.hpp>
#endif

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename P1, typename P2>
void test_null_distance_3d()
{
    P1 p1;
    bg::assign_values(p1, 1, 2, 3);
    P2 p2;
    bg::assign_values(p2, 1, 2, 3);

    typedef bg::strategy::distance::pythagoras<> pythagoras_type;
    typedef typename bg::strategy::distance::services::return_type<pythagoras_type, P1, P2>::type return_type;

    pythagoras_type pythagoras;
    return_type result = pythagoras.apply(p1, p2);

    BOOST_CHECK_EQUAL(result, return_type(0));
}

template <typename P1, typename P2>
void test_axis_3d()
{
    P1 p1;
    bg::assign_values(p1, 0, 0, 0);
    P2 p2;
    bg::assign_values(p2, 1, 0, 0);

    typedef bg::strategy::distance::pythagoras<> pythagoras_type;
    typedef typename bg::strategy::distance::services::return_type<pythagoras_type, P1, P2>::type return_type;

    pythagoras_type pythagoras;

    return_type result = pythagoras.apply(p1, p2);
    BOOST_CHECK_EQUAL(result, return_type(1));

    bg::assign_values(p2, 0, 1, 0);
    result = pythagoras.apply(p1, p2);
    BOOST_CHECK_EQUAL(result, return_type(1));

    bg::assign_values(p2, 0, 0, 1);
    result = pythagoras.apply(p1, p2);
    BOOST_CHECK_CLOSE(result, return_type(1), 0.001);
}

template <typename P1, typename P2>
void test_arbitrary_3d()
{
    P1 p1;
    bg::assign_values(p1, 1, 2, 3);
    P2 p2;
    bg::assign_values(p2, 9, 8, 7);

    {
        typedef bg::strategy::distance::pythagoras<> strategy_type;
        typedef typename bg::strategy::distance::services::return_type<strategy_type, P1, P2>::type return_type;

        strategy_type strategy;
        return_type result = strategy.apply(p1, p2);
        BOOST_CHECK_CLOSE(result, return_type(10.77032961427), 0.001);
    }

    {
        // Check comparable distance
        typedef bg::strategy::distance::comparable::pythagoras<> strategy_type;
        typedef typename bg::strategy::distance::services::return_type<strategy_type, P1, P2>::type return_type;

        strategy_type strategy;
        return_type result = strategy.apply(p1, p2);
        BOOST_CHECK_EQUAL(result, return_type(116));
    }
}

template <typename P1, typename P2, typename CalculationType>
void test_services()
{
    namespace bgsd = bg::strategy::distance;
    namespace services = bg::strategy::distance::services;

    {

        // Compile-check if there is a strategy for this type
        typedef typename services::default_strategy<bg::point_tag, P1, P2>::type pythagoras_strategy_type;
    }


    P1 p1;
    bg::assign_values(p1, 1, 2, 3);

    P2 p2;
    bg::assign_values(p2, 4, 5, 6);

    double const sqr_expected = 3*3 + 3*3 + 3*3; // 27
    double const expected = sqrt(sqr_expected); // sqrt(27)=5.1961524227

    // 1: normal, calculate distance:

    typedef bgsd::pythagoras<CalculationType> strategy_type;

    BOOST_CONCEPT_ASSERT( (bg::concept::PointDistanceStrategy<strategy_type, P1, P2>) );

    typedef typename bgsd::services::return_type<strategy_type, P1, P2>::type return_type;

    strategy_type strategy;
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

    return_type c_result = comparable.apply(p1, p2);
    BOOST_CHECK_CLOSE(c_result, return_type(sqr_expected), 0.001);

    // 4: the comparable_type should have a distance_strategy_constructor as well,
    //    knowing how to compare something with a fixed distance
    return_type c_dist5 = services::result_from_distance<comparable_type, P1, P2>::apply(comparable, 5.0);
    return_type c_dist6 = services::result_from_distance<comparable_type, P1, P2>::apply(comparable, 6.0);

    // If this is the case:
    BOOST_CHECK(c_dist5 < c_result && c_result < c_dist6);

    // This should also be the case
    return_type dist5 = services::result_from_distance<strategy_type, P1, P2>::apply(strategy, 5.0);
    return_type dist6 = services::result_from_distance<strategy_type, P1, P2>::apply(strategy, 6.0);
    BOOST_CHECK(dist5 < result && result < dist6);
}


template <typename CoordinateType, typename CalculationType, typename AssignType>
void test_big_2d_with(AssignType const& x1, AssignType const& y1,
                 AssignType const& x2, AssignType const& y2)
{
    typedef bg::model::point<CoordinateType, 2, bg::cs::cartesian> point_type;
    typedef bg::strategy::distance::pythagoras<CalculationType> pythagoras_type;

    pythagoras_type pythagoras;
    typedef typename bg::strategy::distance::services::return_type<pythagoras_type, point_type, point_type>::type return_type;


    point_type p1, p2;
    bg::assign_values(p1, x1, y1);
    bg::assign_values(p2, x2, y2);
    return_type d = pythagoras.apply(p1, p2);

    /***
    std::cout << typeid(CalculationType).name()
        << " " << std::fixed << std::setprecision(20) << d
        << std::endl << std::endl;
    ***/


    BOOST_CHECK_CLOSE(d, return_type(1076554.5485833955678294387789057), 0.001);
}

template <typename CoordinateType, typename CalculationType>
void test_big_2d()
{
    test_big_2d_with<CoordinateType, CalculationType>
        (123456.78900001, 234567.89100001,
        987654.32100001, 876543.21900001);
}

template <typename CoordinateType, typename CalculationType>
void test_big_2d_string()
{
    test_big_2d_with<CoordinateType, CalculationType>
        ("123456.78900001", "234567.89100001",
        "987654.32100001", "876543.21900001");
}

template <typename CoordinateType>
void test_integer(bool check_types)
{
    typedef bg::model::point<CoordinateType, 2, bg::cs::cartesian> point_type;

    point_type p1, p2;
    bg::assign_values(p1, 12345678, 23456789);
    bg::assign_values(p2, 98765432, 87654321);

    typedef bg::strategy::distance::pythagoras<> pythagoras_type;
    pythagoras_type pythagoras;
    BOOST_AUTO(distance, pythagoras.apply(p1, p2));
    BOOST_CHECK_CLOSE(distance, 107655455.02347542, 0.001);

    typedef typename bg::strategy::distance::services::comparable_type
        <
            pythagoras_type
        >::type comparable_type;
    comparable_type comparable;
    BOOST_AUTO(cdistance, comparable.apply(p1, p2));
    BOOST_CHECK_EQUAL(cdistance, 11589696996311540);

    typedef BOOST_TYPEOF(cdistance) cdistance_type;
    typedef BOOST_TYPEOF(distance) distance_type;

    distance_type distance2 = sqrt(distance_type(cdistance));
    BOOST_CHECK_CLOSE(distance, distance2, 0.001);

    if (check_types)
    {
        BOOST_CHECK((boost::is_same<distance_type, double>::type::value));
        BOOST_CHECK((boost::is_same<cdistance_type, boost::long_long_type>::type::value));
    }
}


template <typename P1, typename P2>
void test_all_3d()
{
    test_null_distance_3d<P1, P2>();
    test_axis_3d<P1, P2>();
    test_arbitrary_3d<P1, P2>();
}

template <typename P>
void test_all_3d()
{
    test_all_3d<P, int[3]>();
    test_all_3d<P, float[3]>();
    test_all_3d<P, double[3]>();
    test_all_3d<P, test::test_point>();
    test_all_3d<P, bg::model::point<int, 3, bg::cs::cartesian> >();
    test_all_3d<P, bg::model::point<float, 3, bg::cs::cartesian> >();
    test_all_3d<P, bg::model::point<double, 3, bg::cs::cartesian> >();
}

template <typename P, typename Strategy>
void time_compare_s(int const n)
{
    boost::timer t;
    P p1, p2;
    bg::assign_values(p1, 1, 1);
    bg::assign_values(p2, 2, 2);
    Strategy strategy;
    typename bg::strategy::distance::services::return_type<Strategy, P, P>::type s = 0;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            bg::set<0>(p2, bg::get<0>(p2) + 0.001);
            s += strategy.apply(p1, p2);
        }
    }
    std::cout << "s: " << s << " t: " << t.elapsed() << std::endl;
}

template <typename P>
void time_compare(int const n)
{
    time_compare_s<P, bg::strategy::distance::pythagoras<> >(n);
    time_compare_s<P, bg::strategy::distance::comparable::pythagoras<> >(n);
}

int test_main(int, char* [])
{
    test_integer<int>(true);
    test_integer<boost::long_long_type>(true);
    test_integer<double>(false);

    test_all_3d<int[3]>();
    test_all_3d<float[3]>();
    test_all_3d<double[3]>();

    test_all_3d<test::test_point>();

    test_all_3d<bg::model::point<int, 3, bg::cs::cartesian> >();
    test_all_3d<bg::model::point<float, 3, bg::cs::cartesian> >();
    test_all_3d<bg::model::point<double, 3, bg::cs::cartesian> >();

    test_big_2d<float, float>();
    test_big_2d<double, double>();
    test_big_2d<long double, long double>();
    test_big_2d<float, long double>();

    test_services<bg::model::point<float, 3, bg::cs::cartesian>, double[3], long double>();
    test_services<double[3], test::test_point, float>();


    // TODO move this to another non-unit test
    // time_compare<bg::model::point<double, 2, bg::cs::cartesian> >(10000);

#if defined(HAVE_TTMATH)

    typedef ttmath::Big<1,4> tt;
    typedef bg::model::point<tt, 3, bg::cs::cartesian> tt_point;

    //test_all_3d<tt[3]>();
    test_all_3d<tt_point>();
    test_all_3d<tt_point, tt_point>();
    test_big_2d<tt, tt>();
    test_big_2d_string<tt, tt>();
#endif
    return 0;
}
