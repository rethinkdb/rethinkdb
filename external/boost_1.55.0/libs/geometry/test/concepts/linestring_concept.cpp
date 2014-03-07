// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <deque>
#include <vector>

#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/append.hpp>
#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/concepts/linestring_concept.hpp>

#include <boost/geometry/io/dsv/write.hpp>


#include <test_common/test_point.hpp>
#include <test_geometries/all_custom_linestring.hpp>
#include <test_geometries/wrapped_boost_array.hpp>



template <typename Geometry>
void test_linestring()
{
    BOOST_CONCEPT_ASSERT( (bg::concept::Linestring<Geometry>) );
    BOOST_CONCEPT_ASSERT( (bg::concept::ConstLinestring<Geometry>) );

    Geometry geometry;
    typedef typename bg::point_type<Geometry>::type P;

    bg::clear(geometry);
    BOOST_CHECK_EQUAL(boost::size(geometry), 0);

    bg::append(geometry, bg::make<P>(1, 2));
    BOOST_CHECK_EQUAL(boost::size(geometry), 1);

    bg::append(geometry, bg::make<P>(3, 4));
    BOOST_CHECK_EQUAL(boost::size(geometry), 2);

    bg::traits::resize<Geometry>::apply(geometry, 1);
    BOOST_CHECK_EQUAL(boost::size(geometry), 1);

    //std::cout << bg::dsv(geometry) << std::endl;
    P p = *boost::begin(geometry);
    //std::cout << bg::dsv(p) << std::endl;
    BOOST_CHECK_EQUAL(bg::get<0>(p), 1);
    BOOST_CHECK_EQUAL(bg::get<1>(p), 2);

    bg::clear(geometry);
    BOOST_CHECK_EQUAL(boost::size(geometry), 0);
}

template <typename Point>
void test_all()
{
    test_linestring<bg::model::linestring<Point> >();
    test_linestring<test::wrapped_boost_array<Point, 10> >();
    test_linestring<all_custom_linestring<Point> >();
}

int test_main(int, char* [])
{
    test_all<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();

    return 0;
}
