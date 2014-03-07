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

#include <boost/geometry/algorithms/make.hpp>


#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <test_common/test_point.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename P>
bg::model::box<P> create_box()
{
    P p1;
    P p2;
    bg::assign_values(p1, 1, 2, 5);
    bg::assign_values(p2, 3, 4, 6);
    return bg::model::box<P>(p1, p2);
}

template <typename B, typename T>
void check_box(B& to_check,
               T min_x, T min_y, T min_z,
               T max_x, T max_y, T max_z)
{
    BOOST_CHECK_EQUAL(bg::get<0>(to_check.min_corner()), min_x);
    BOOST_CHECK_EQUAL(bg::get<1>(to_check.min_corner()), min_y);
    BOOST_CHECK_EQUAL(bg::get<2>(to_check.min_corner()), min_z);
    BOOST_CHECK_EQUAL(bg::get<0>(to_check.max_corner()), max_x);
    BOOST_CHECK_EQUAL(bg::get<1>(to_check.max_corner()), max_y);
    BOOST_CHECK_EQUAL(bg::get<2>(to_check.max_corner()), max_z);
}

template <typename P>
void test_construction()
{
    typedef typename bg::coordinate_type<P>::type T;

    bg::model::box<P> b1 = bg::make_zero<bg::model::box<P> >();
    check_box(b1, T(),T(),T(),T(),T(),T());

    bg::model::box<P> b2(create_box<P>());
    check_box(b2, 1,2,5,3,4,6);

    bg::model::box<P> b3 = bg::make_inverse<bg::model::box<P> >();
    check_box(b3, boost::numeric::bounds<T>::highest(),
                  boost::numeric::bounds<T>::highest(),
                  boost::numeric::bounds<T>::highest(),
                  boost::numeric::bounds<T>::lowest(),
                  boost::numeric::bounds<T>::lowest(),
                  boost::numeric::bounds<T>::lowest());
}

template <typename P>
void test_assignment()
{
    bg::model::box<P> b(create_box<P>());
    bg::set<0>(b.min_corner(), 10);
    bg::set<1>(b.min_corner(), 20);
    bg::set<2>(b.min_corner(), 30);
    bg::set<0>(b.max_corner(), 40);
    bg::set<1>(b.max_corner(), 50);
    bg::set<2>(b.max_corner(), 60);
    check_box(b, 10,20,30,40,50,60);
}

template <typename P>
void test_all()
{
    test_construction<P>();
    test_assignment<P>();
}

int test_main(int, char* [])
{
    test_all<int[3]>();
    test_all<float[3]>();
    test_all<double[3]>();
    test_all<test::test_point>();
    test_all<bg::model::point<int, 3, bg::cs::cartesian> >();
    test_all<bg::model::point<float, 3, bg::cs::cartesian> >();
    test_all<bg::model::point<double, 3, bg::cs::cartesian> >();

    return 0;
}
