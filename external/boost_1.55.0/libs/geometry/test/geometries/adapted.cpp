// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <deque>
#include <vector>

#include <geometry_test_common.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/multi/multi.hpp>

#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

#include <test_common/test_point.hpp>

#define BOOST_GEOMETRY_TEST_RING



#if defined(BOOST_GEOMETRY_TEST_RING)

    #include <boost/geometry/geometries/register/ring.hpp>
    #include <boost/geometry/geometries/concepts/ring_concept.hpp>

    BOOST_GEOMETRY_REGISTER_RING_TEMPLATED(std::vector)
    BOOST_GEOMETRY_REGISTER_RING_TEMPLATED(std::deque)
  
#elif defined(BOOST_GEOMETRY_TEST_MULTI_POINT)

    #include <boost/geometry/multi/geometries/register/multi_point.hpp>
    #include <boost/geometry/multi/geometries/concepts/multi_point_concept.hpp>

    BOOST_GEOMETRY_REGISTER_MULTI_POINT_TEMPLATED(std::vector)
    BOOST_GEOMETRY_REGISTER_MULTI_POINT_TEMPLATED(std::deque)
  
#else

    #include <boost/geometry/geometries/register/linestring.hpp>
    #include <boost/geometry/geometries/concepts/linestring_concept.hpp>

    BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::vector)
    BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::deque)
  
#endif


BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


// ----------------------------------------------------------------------------

template <typename G>
void test_geometry(G const& geometry, std::size_t expected_size = 0)
{
#if defined(BOOST_GEOMETRY_TEST_RING)
    BOOST_CONCEPT_ASSERT( (bg::concept::ConstRing<G>) );
#elif defined(BOOST_GEOMETRY_TEST_MULTI_POINT)
    BOOST_CONCEPT_ASSERT( (bg::concept::ConstMultiPoint<G>) );
#else
    BOOST_CONCEPT_ASSERT( (bg::concept::ConstLinestring<G>) );
#endif

    typedef typename bg::point_type<G>::type P;
    typedef typename bg::coordinate_type<P>::type C;

    // Check range-like behaviour
    BOOST_CHECK_EQUAL(boost::size(geometry), expected_size);

}

template <typename P>
void test_all()
{
    test_geometry(std::vector<P>());
    test_geometry(std::deque<P>());
    //test_geometry(std::list<P>());

    /***
    double vertices[][3] = {
        {-1, -1,  1}, {1, -1,  1}, {1, 1,  1}, {-1, 1,  1},
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}
        };

    test_geometry(vertices, 8);
    ***/
}

int test_main(int, char* [])
{
    test_all<test::test_point>();
    test_all<boost::tuple<float, float> >();
    test_all<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<long double, 2, bg::cs::cartesian> >();

    test_all<boost::tuple<float, float, float> >();
    test_all<bg::model::point<double, 3, bg::cs::cartesian> >();
    test_all<bg::model::point<long double, 3, bg::cs::cartesian> >();

    test_all<boost::tuple<float, float, float, float, float> >();

    return 0;
}
