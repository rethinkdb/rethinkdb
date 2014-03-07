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

#include <deque>
#include <vector>

#include <boost/concept/requires.hpp>
#include <geometry_test_common.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/algorithms/append.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>
#include <boost/variant/variant.hpp>

#include <test_common/test_point.hpp>
#include <test_geometries/wrapped_boost_array.hpp>

BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::vector)
BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::deque)


template <typename G>
void test_geometry(G& geometry, bool check)
{
    typedef typename bg::point_type<G>::type P;

    bg::append(geometry, bg::make_zero<P>());
    if (check)
    {
        BOOST_CHECK_EQUAL(bg::num_points(geometry), 1u);
    }

    // Append a range
    std::vector<P> v;
    v.push_back(bg::make_zero<P>());
    v.push_back(bg::make_zero<P>());
    bg::append(geometry, v);

    if (check)
    {
        BOOST_CHECK_EQUAL(bg::num_points(geometry), 3u);
    }

    bg::clear(geometry);

    if (check)
    {
        BOOST_CHECK_EQUAL(bg::num_points(geometry), 0u);
    }

    //P p = boost::range::front(geometry);
}

template <typename G>
void test_geometry_and_variant(bool check = true)
{
    G geometry;
    boost::variant<G> variant_geometry = G();
    test_geometry(geometry, check);
    test_geometry(variant_geometry, check);
}

template <typename P>
void test_all()
{
    test_geometry_and_variant<P>(false);
    test_geometry_and_variant<bg::model::box<P> >(false);
    test_geometry_and_variant<bg::model::segment<P> >(false);
    test_geometry_and_variant<bg::model::linestring<P> >();
    test_geometry_and_variant<bg::model::ring<P> >();
    test_geometry_and_variant<bg::model::polygon<P> >();

    test_geometry_and_variant<std::vector<P> >();
    test_geometry_and_variant<std::deque<P> >();
    //test_geometry_and_variant<std::list<P> >();
}

int test_main(int, char* [])
{
    test_all<test::test_point>();
    test_all<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::point<ttmath_big, 2, bg::cs::cartesian> >();
#endif

    return 0;
}
