// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_DISTANCE_HPP
#define BOOST_GEOMETRY_TEST_DISTANCE_HPP

#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/strategies/strategies.hpp>


// Define a custom distance strategy
// For this one, the "taxicab" distance,
// see http://en.wikipedia.org/wiki/Taxicab_geometry

// For a point-point-distance operation, one typename Point is enough.
// For a point-segment-distance operation, there is some magic inside
// using another point type and casting if necessary. Therefore,
// two point-types are necessary.
struct taxicab_distance
{
    template <typename P1, typename P2>
    static inline typename bg::coordinate_type<P1>::type apply(
                    P1 const& p1, P2 const& p2)
    {
        using bg::get;
        using bg::math::abs;
        return abs(get<0>(p1) - get<1>(p2))
            + abs(get<1>(p1) - get<1>(p2));
    }
};



namespace boost { namespace geometry { namespace strategy { namespace distance { namespace services
{

template <>
struct tag<taxicab_distance>
{
    typedef strategy_tag_distance_point_point type;
};


template <typename P1, typename P2>
struct return_type<taxicab_distance, P1, P2>
{
    typedef typename coordinate_type<P1>::type type;
};


template <>
struct comparable_type<taxicab_distance>
{
    typedef taxicab_distance type;
};

template <>
struct get_comparable<taxicab_distance>
{
    static inline taxicab_distance apply(taxicab_distance const& input)
    {
        return input;
    }
};

template <typename P1, typename P2>
struct result_from_distance<taxicab_distance, P1, P2>
{
    template <typename T>
    static inline typename coordinate_type<P1>::type apply(taxicab_distance const& , T const& value)
    {
        return value;
    }
};


}}}}} // namespace bg::strategy::distance::services





template <typename Geometry1, typename Geometry2>
void test_distance(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            long double expected_distance)
{
    typename bg::default_distance_result<Geometry1>::type distance = bg::distance(geometry1, geometry2);

#ifdef GEOMETRY_TEST_DEBUG
    std::ostringstream out;
    out << typeid(typename bg::coordinate_type<Geometry1>::type).name()
        << std::endl
        << typeid(typename bg::default_distance_result<Geometry1>::type).name()
        << std::endl
        << "distance : " << bg::distance(geometry1, geometry2)
        << std::endl;
    std::cout << out.str();
#endif

    BOOST_CHECK_CLOSE(distance, expected_distance, 0.0001);
}


template <typename Geometry1, typename Geometry2>
void test_geometry(std::string const& wkt1, std::string const& wkt2, double expected_distance)
{
    Geometry1 geometry1;
    bg::read_wkt(wkt1, geometry1);
    Geometry2 geometry2;
    bg::read_wkt(wkt2, geometry2);

    test_distance(geometry1, geometry2, expected_distance);
}

template <typename Geometry1, typename Geometry2>
void test_empty_input(Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    try
    {
        bg::distance(geometry1, geometry2);
    }
    catch(bg::empty_input_exception const& )
    {
        return;
    }
    BOOST_CHECK_MESSAGE(false, "A empty_input_exception should have been thrown" );
}


#endif
