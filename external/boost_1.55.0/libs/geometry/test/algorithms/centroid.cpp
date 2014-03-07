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

#include <algorithms/test_centroid.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

#include <test_geometries/all_custom_polygon.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename Polygon>
void test_polygon()
{
    test_centroid<Polygon>(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2"
        ",3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3))",
        4.06923363095238, 1.65055803571429);

    // with holes
    test_centroid<Polygon>(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2"
        ",3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
        ",(4 2,4.2 1.4,4.8 1.9,4.4 2.2,4 2))"
        ,
        4.0466264962959677, 1.6348996057331333);

}


template <typename P>
void test_2d()
{
    test_centroid<bg::model::linestring<P> >("LINESTRING(1 1, 2 2, 3 3)", 2.0, 2.0);
    test_centroid<bg::model::linestring<P> >("LINESTRING(0 0,0 4, 4 4)", 1.0, 3.0);
    test_centroid<bg::model::linestring<P> >("LINESTRING(0 0,3 3,0 6,3 9,0 12)", 1.5, 6.0);

    test_centroid<bg::model::segment<P> >("LINESTRING(1 1, 3 3)", 2.0, 2.0);

    test_centroid<bg::model::ring<P> >(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2"
        ",3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3))",
        4.06923363095238, 1.65055803571429);

    test_polygon<bg::model::polygon<P> >();
    test_polygon<all_custom_polygon<P> >();

    // ccw
    test_centroid<bg::model::ring<P, false> >(
        "POLYGON((2 1.3,2.9 0.7,4.9 0.8,5.4 1.2,5.3 2.6,4.1 3,3.4 2"
            ",3.7 1.6,3.4 1.2,2.8 1.8,2.4 1.7,2 1.3))",
        4.06923363095238, 1.65055803571429);

    // open / closed
    test_centroid<bg::model::ring<P, true, true> >(
            "POLYGON((1 1,2 2,3 1,2 0,1 1))", 2.0, 1.0);
    test_centroid<bg::model::ring<P, true, false> >(
            "POLYGON((1 1,2 2,3 1,2 0))", 2.0, 1.0);

    test_centroid<bg::model::box<P> >("POLYGON((1 2,3 4))", 2, 3);
    test_centroid<P>("POINT(3 3)", 3, 3);
}


template <typename P>
void test_3d()
{
    test_centroid<bg::model::linestring<P> >("LINESTRING(1 2 3,4 5 -6,7 -8 9,-10 11 12,13 -14 -15, 16 17 18)",
                                             5.6748865168734692, 0.31974938587214002, 1.9915270387763671);
    test_centroid<bg::model::box<P> >("POLYGON((1 2 3,5 6 7))", 3, 4, 5);
    test_centroid<bg::model::segment<P> >("LINESTRING(1 1 1,3 3 3)", 2, 2, 2);
    test_centroid<P>("POINT(1 2 3)", 1, 2, 3);
}


template <typename P>
void test_5d()
{
    test_centroid<bg::model::linestring<P> >("LINESTRING(1 2 3 4 95,4 5 -6 24 40,7 -8 9 -5 -7,-10 11 12 -5 5,13 -14 -15 4 3, 16 17 18 5 12)",
                                             4.9202312983547678, 0.69590937869808345, 1.2632138719797417, 6.0468332057401986, 23.082402715244868);
}

template <typename P>
void test_exceptions()
{
    test_centroid_exception<bg::model::linestring<P> >();
    test_centroid_exception<bg::model::polygon<P> >();
    test_centroid_exception<bg::model::ring<P> >();
}

void test_large_integers()
{
    typedef bg::model::point<int, 2, bg::cs::cartesian> int_point_type;
    typedef bg::model::point<double, 2, bg::cs::cartesian> double_point_type;

    bg::model::polygon<int_point_type> int_poly;
    bg::model::polygon<double_point_type> double_poly;

    std::string const polygon_li = "POLYGON((1872000 528000,1872000 192000,1536119 192000,1536000 528000,1200000 528000,1200000 863880,1536000 863880,1872000 863880,1872000 528000))";
    bg::read_wkt(polygon_li, int_poly);
    bg::read_wkt(polygon_li, double_poly);

    int_point_type int_centroid;
    double_point_type double_centroid;

    bg::centroid(int_poly, int_centroid);
    bg::centroid(double_poly, double_centroid);

    int_point_type double_centroid_as_int;
    bg::assign(int_centroid, double_centroid_as_int);

    BOOST_CHECK_EQUAL(bg::get<0>(int_centroid), bg::get<0>(double_centroid_as_int));
    BOOST_CHECK_EQUAL(bg::get<1>(int_centroid), bg::get<1>(double_centroid_as_int));
}


int test_main(int, char* [])
{
    test_2d<bg::model::d2::point_xy<double> >();
    test_2d<boost::tuple<float, float> >();
    test_2d<bg::model::d2::point_xy<float> >();

    test_3d<boost::tuple<double, double, double> >();

    test_5d<boost::tuple<double, double, double, double, double> >();

#if defined(HAVE_TTMATH)
    test_2d<bg::model::d2::point_xy<ttmath_big> >();
    test_3d<boost::tuple<ttmath_big, ttmath_big, ttmath_big> >();
#endif

    test_large_integers();
    test_exceptions<bg::model::d2::point_xy<double> >();

    return 0;
}
