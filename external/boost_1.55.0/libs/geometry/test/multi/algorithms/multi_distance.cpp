// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <string>


#include <geometry_test_common.hpp>

#include <algorithms/test_distance.hpp>

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/io/wkt/read.hpp>

#include <boost/geometry/strategies/strategies.hpp>


#include <boost/geometry/multi/algorithms/distance.hpp>
#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/geometry/multi/io/wkt/read.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <test_common/test_point.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename Geometry1, typename Geometry2>
void test_distance(std::string const& wkt1, std::string const& wkt2, double expected)
{
    Geometry1 g1;
    Geometry2 g2;
    bg::read_wkt(wkt1, g1);
    bg::read_wkt(wkt2, g2);
    typename bg::default_distance_result<Geometry1, Geometry2>::type d = bg::distance(g1, g2);

    BOOST_CHECK_CLOSE(d, expected, 0.0001);
}

template <typename Geometry1, typename Geometry2, typename Strategy>
void test_distance(Strategy const& strategy, std::string const& wkt1,
                   std::string const& wkt2, double expected)
{
    Geometry1 g1;
    Geometry2 g2;
    bg::read_wkt(wkt1, g1);
    bg::read_wkt(wkt2, g2);
    typename bg::default_distance_result<Geometry1, Geometry2>::type d = bg::distance(g1, g2, strategy);

    BOOST_CHECK_CLOSE(d, expected, 0.0001);
}


template <typename P>
void test_2d()
{
    typedef bg::model::multi_point<P> mp;
    typedef bg::model::multi_linestring<bg::model::linestring<P> > ml;
    test_distance<P, P>("POINT(0 0)", "POINT(1 1)", sqrt(2.0));
    test_distance<P, mp>("POINT(0 0)", "MULTIPOINT((1 1),(1 0),(0 2))", 1.0);
    test_distance<mp, P>("MULTIPOINT((1 1),(1 0),(0 2))", "POINT(0 0)", 1.0);
    test_distance<mp, mp>("MULTIPOINT((1 1),(1 0),(0 2))", "MULTIPOINT((2 2),(2 3))", sqrt(2.0));
    test_distance<P, ml>("POINT(0 0)", "MULTILINESTRING((1 1,2 2),(1 0,2 0),(0 2,0 3))", 1.0);
    test_distance<ml, P>("MULTILINESTRING((1 1,2 2),(1 0,2 0),(0 2,0 3))", "POINT(0 0)", 1.0);
    test_distance<ml, mp>("MULTILINESTRING((1 1,2 2),(1 0,2 0),(0 2,0 3))", "MULTIPOINT((0 0),(1 1))", 0.0);

    // Test with a strategy
    bg::strategy::distance::pythagoras<> pyth;
    test_distance<P, P>(pyth, "POINT(0 0)", "POINT(1 1)", sqrt(2.0));
    test_distance<P, mp>(pyth, "POINT(0 0)", "MULTIPOINT((1 1),(1 0),(0 2))", 1.0);
    test_distance<mp, P>(pyth, "MULTIPOINT((1 1),(1 0),(0 2))", "POINT(0 0)", 1.0);
}


template <typename P>
void test_3d()
{
    typedef bg::model::multi_point<P> mp;
    test_distance<P, P>("POINT(0 0 0)", "POINT(1 1 1)", sqrt(3.0));
    test_distance<P, mp>("POINT(0 0 0)", "MULTIPOINT((1 1 1),(1 0 0),(0 1 2))", 1.0);
    test_distance<mp, mp>("MULTIPOINT((1 1 1),(1 0 0),(0 0 2))", "MULTIPOINT((2 2 2),(2 3 4))", sqrt(3.0));
}


template <typename P1, typename P2>
void test_mixed()
{
    typedef bg::model::multi_point<P1> mp1;
    typedef bg::model::multi_point<P2> mp2;

    test_distance<P1, P2>("POINT(0 0)", "POINT(1 1)", sqrt(2.0));

    test_distance<P1, mp1>("POINT(0 0)", "MULTIPOINT((1 1),(1 0),(0 2))", 1.0);
    test_distance<P1, mp2>("POINT(0 0)", "MULTIPOINT((1 1),(1 0),(0 2))", 1.0);
    test_distance<P2, mp1>("POINT(0 0)", "MULTIPOINT((1 1),(1 0),(0 2))", 1.0);
    test_distance<P2, mp2>("POINT(0 0)", "MULTIPOINT((1 1),(1 0),(0 2))", 1.0);

    // Test automatic reversal
    test_distance<mp1, P1>("MULTIPOINT((1 1),(1 0),(0 2))", "POINT(0 0)", 1.0);
    test_distance<mp1, P2>("MULTIPOINT((1 1),(1 0),(0 2))", "POINT(0 0)", 1.0);
    test_distance<mp2, P1>("MULTIPOINT((1 1),(1 0),(0 2))", "POINT(0 0)", 1.0);
    test_distance<mp2, P2>("MULTIPOINT((1 1),(1 0),(0 2))", "POINT(0 0)", 1.0);

    // Test multi-multi using different point types for each
    test_distance<mp1, mp2>("MULTIPOINT((1 1),(1 0),(0 2))", "MULTIPOINT((2 2),(2 3))", sqrt(2.0));

    // Test with a strategy
    using namespace bg::strategy::distance;

    test_distance<P1, P2>(pythagoras<>(), "POINT(0 0)", "POINT(1 1)", sqrt(2.0));

    test_distance<P1, mp1>(pythagoras<>(), "POINT(0 0)", "MULTIPOINT((1 1),(1 0),(0 2))", 1.0);
    test_distance<P1, mp2>(pythagoras<>(), "POINT(0 0)", "MULTIPOINT((1 1),(1 0),(0 2))", 1.0);
    test_distance<P2, mp1>(pythagoras<>(), "POINT(0 0)", "MULTIPOINT((1 1),(1 0),(0 2))", 1.0);
    test_distance<P2, mp2>(pythagoras<>(), "POINT(0 0)", "MULTIPOINT((1 1),(1 0),(0 2))", 1.0);

    // Most interesting: reversal AND a strategy (note that the stategy must be reversed automatically
    test_distance<mp1, P1>(pythagoras<>(), "MULTIPOINT((1 1),(1 0),(0 2))", "POINT(0 0)", 1.0);
    test_distance<mp1, P2>(pythagoras<>(), "MULTIPOINT((1 1),(1 0),(0 2))", "POINT(0 0)", 1.0);
    test_distance<mp2, P1>(pythagoras<>(), "MULTIPOINT((1 1),(1 0),(0 2))", "POINT(0 0)", 1.0);
    test_distance<mp2, P2>(pythagoras<>(), "MULTIPOINT((1 1),(1 0),(0 2))", "POINT(0 0)", 1.0);
}

template <typename P>
void test_empty_input()
{
    P p;
    bg::model::multi_point<P> mp_empty;
    bg::model::multi_linestring<bg::model::linestring<P> > ml_empty;

    test_empty_input(p, mp_empty);
    test_empty_input(p, ml_empty);
    test_empty_input(mp_empty, mp_empty);

    // Test behaviour if one of the inputs is empty
    bg::model::multi_point<P> mp;
    mp.push_back(p);
    test_empty_input(mp_empty, mp);
    test_empty_input(mp, mp_empty);
}


int test_main( int , char* [] )
{
    test_2d<boost::tuple<float, float> >();
    test_2d<bg::model::d2::point_xy<float> >();
    test_2d<bg::model::d2::point_xy<double> >();

    test_3d<boost::tuple<float, float, float> >();
    test_3d<bg::model::point<double, 3, bg::cs::cartesian> >();

    test_mixed<bg::model::d2::point_xy<float>, bg::model::d2::point_xy<double> >();

#ifdef HAVE_TTMATH
    test_2d<bg::model::d2::point_xy<ttmath_big> >();
    test_mixed<bg::model::d2::point_xy<ttmath_big>, bg::model::d2::point_xy<double> >();
#endif

    test_empty_input<bg::model::d2::point_xy<int> >();

    return 0;
}
