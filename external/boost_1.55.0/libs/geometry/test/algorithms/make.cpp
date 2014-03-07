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

#include <boost/geometry/io/wkt/write.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <test_common/test_point.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename T, typename P>
void test_point_2d()
{
    P p = bg::make<P>((T) 123, (T) 456);
    BOOST_CHECK_CLOSE(bg::get<0>(p), 123.0, 1.0e-6);
    BOOST_CHECK_CLOSE(bg::get<1>(p), 456.0, 1.0e-6);
}

template <typename T, typename P>
void test_point_3d()
{
    P p = bg::make<P>((T) 123, (T) 456, (T) 789);
    BOOST_CHECK_CLOSE( bg::get<0>(p), 123.0, 1.0e-6);
    BOOST_CHECK_CLOSE( bg::get<1>(p), 456.0, 1.0e-6);
    BOOST_CHECK_CLOSE( bg::get<2>(p), 789.0, 1.0e-6);
}

template <typename T, typename P>
void test_box_2d()
{
    typedef bg::model::box<P> B;
    B b = bg::make<B>((T) 123, (T) 456, (T) 789, (T) 1011);
    BOOST_CHECK_CLOSE( (bg::get<bg::min_corner, 0>(b)), 123.0, 1.0e-6);
    BOOST_CHECK_CLOSE( (bg::get<bg::min_corner, 1>(b)), 456.0, 1.0e-6);
    BOOST_CHECK_CLOSE( (bg::get<bg::max_corner, 0>(b)), 789.0, 1.0e-6);
    BOOST_CHECK_CLOSE( (bg::get<bg::max_corner, 1>(b)), 1011.0, 1.0e-6);

    b = bg::make_inverse<B>();
}

template <typename T, typename P>
void test_linestring_2d()
{
    typedef bg::model::linestring<P> L;

    T coors[][2] = {{1,2}, {3,4}};

    L line = bg::detail::make::make_points<L>(coors);

    BOOST_CHECK_EQUAL(line.size(), 2u);
}

template <typename T, typename P>
void test_linestring_3d()
{
    typedef bg::model::linestring<P> L;

    T coors[][3] = {{1,2,3}, {4,5,6}};

    L line = bg::detail::make::make_points<L>(coors);

    BOOST_CHECK_EQUAL(line.size(), 2u);
    //std::cout << dsv(line) << std::endl;

}

template <typename T, typename P>
void test_2d_t()
{
    test_point_2d<T, P>();
    test_box_2d<T, P>();
    test_linestring_2d<T, P>();
}

template <typename P>
void test_2d()
{
    test_2d_t<int, P>();
    test_2d_t<float, P>();
    test_2d_t<double, P>();
}

template <typename T, typename P>
void test_3d_t()
{
    test_linestring_3d<T, P>();
//  test_point_3d<T, test_point>();
}

template <typename P>
void test_3d()
{
    test_3d_t<int, P>();
    test_3d_t<float, P>();
    test_3d_t<double, P>();
}

int test_main(int, char* [])
{
    //test_2d<int[2]>();
    //test_2d<float[2]>();
    //test_2d<double[2]>();
    test_2d<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_2d<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_2d<bg::model::point<double, 2, bg::cs::cartesian> >();


    test_3d<bg::model::point<double, 3, bg::cs::cartesian> >();

#if defined(HAVE_TTMATH)
    test_2d<bg::model::point<ttmath_big, 2, bg::cs::cartesian> >();
    test_3d<bg::model::point<ttmath_big, 3, bg::cs::cartesian> >();
#endif


    return 0;
}
