// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef GEOMETRY_TEST_COMMON_WITH_POINTER_HPP
#define GEOMETRY_TEST_COMMON_WITH_POINTER_HPP


#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/tag.hpp>

// NOTE: since Boost 1.51 the Point type may always be a pointer.
// Therefore the traits class don't need to add a pointer.
// This obsoletes this whole test-point-type



namespace test
{

// Sample point, having x/y
struct test_point_xy
{
    float x,y;
};

}


namespace boost { namespace geometry { namespace traits {

template<> struct tag<test::test_point_xy>
{ typedef point_tag type; };

template<> struct coordinate_type<test::test_point_xy>
{ typedef double type; };

template<> struct coordinate_system<test::test_point_xy>
{ typedef cs::cartesian type; };

template<> struct dimension<test::test_point_xy> : boost::mpl::int_<2> {};

template<>
struct access<test::test_point_xy, 0>
{
    static double get(test::test_point_xy const& p)
    {
        return p.x;
    }

    static void set(test::test_point_xy& p, double const& value)
    {
        p.x = value;
    }

};


template<>
struct access<test::test_point_xy, 1>
{
    static double get(test::test_point_xy const& p)
    {
        return p.y;
    }

    static void set(test::test_point_xy& p, double const& value)
    {
        p.y = value;
    }

};

}}} // namespace bg::traits


#endif // #ifndef GEOMETRY_TEST_COMMON_WITH_POINTER_HPP
