// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef GEOMETRY_TEST_TEST_COMMON_TEST_POINT_HPP
#define GEOMETRY_TEST_TEST_COMMON_TEST_POINT_HPP

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

// Test point class

struct test_point
{
    float c1, c2, c3;
};


} // namespace test


namespace boost { namespace geometry { namespace traits {

template<>
struct tag<test::test_point> { typedef point_tag type; };

template<>
struct coordinate_type<test::test_point> { typedef float type; };

template<>
struct coordinate_system<test::test_point> { typedef cs::cartesian type; };

template<>
struct dimension<test::test_point>: boost::mpl::int_<3> {};

template<> struct access<test::test_point, 0>
{
    static inline const float& get(const test::test_point& p)
    {
        return p.c1;
    }

    static inline void set(test::test_point& p, const float& value)
    {
        p.c1 = value;
    }
};

template<> struct access<test::test_point, 1>
{
    static inline const float& get(const test::test_point& p)
    {
        return p.c2;
    }

    static inline void set(test::test_point& p, const float& value)
    {
        p.c2 = value;
    }
};

template<> struct access<test::test_point, 2>
{
    static inline const float& get(const test::test_point& p)
    {
        return p.c3;
    }

    static inline void set(test::test_point& p, const float& value)
    {
        p.c3 = value;
    }
};

}}} // namespace bg::traits

#endif // GEOMETRY_TEST_TEST_COMMON_TEST_POINT_HPP
