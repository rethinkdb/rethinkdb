// Boost.Geometry Index
// Unit Test

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithms/test_margin.hpp>

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

//#define GEOMETRY_TEST_DEBUG

void test_large_integers()
{
    typedef bg::model::point<int, 2, bg::cs::cartesian> int_point_type;
    typedef bg::model::point<double, 2, bg::cs::cartesian> double_point_type;

    bg::model::box<int_point_type> int_box;
    bg::model::box<double_point_type> double_box;

    std::string const box_li = "POLYGON((1536119 192000, 1872000 528000))";
    bg::read_wkt(box_li, int_box);
    bg::read_wkt(box_li, double_box);

    double int_value = bgi::detail::comparable_margin(int_box);
    double double_value = bgi::detail::comparable_margin(double_box);

    BOOST_CHECK_CLOSE(int_value, double_value, 0.0001);
}

int test_main(int, char* [])
{
    typedef bg::model::point<int, 2, bg::cs::cartesian> P2ic;
    typedef bg::model::point<float, 2, bg::cs::cartesian> P2fc;
    typedef bg::model::point<double, 2, bg::cs::cartesian> P2dc;

    typedef bg::model::point<int, 3, bg::cs::cartesian> P3ic;
    typedef bg::model::point<float, 3, bg::cs::cartesian> P3fc;
    typedef bg::model::point<double, 3, bg::cs::cartesian> P3dc;
    
    test_geometry<bg::model::box<P2ic> >("POLYGON((0 1,2 4))", 5);
    test_geometry<bg::model::box<P2fc> >("POLYGON((0 1,2 4))", 5.0);
    test_geometry<bg::model::box<P2dc> >("POLYGON((0 1,2 4))", 5.0);
    test_geometry<bg::model::box<P3ic> >("POLYGON((0 1 2,2 4 6))", 9);
    test_geometry<bg::model::box<P3fc> >("POLYGON((0 1 2,2 4 6))", 9.0);
    test_geometry<bg::model::box<P3dc> >("POLYGON((0 1 2,2 4 6))", 9.0);
    
#ifdef HAVE_TTMATH
    typedef bg::model::point<ttmath_big, 2, bg::cs::cartesian> P2ttmc;
    typedef bg::model::point<ttmath_big, 3, bg::cs::cartesian> P3ttmc;

    test_geometry<bg::model::box<P2ttmc> >("POLYGON((0 1,2 4))", 10.0);
    test_geometry<bg::model::box<P3ttmc> >("POLYGON((0 1 2,2 4 6))", 52.0);
#endif

    test_large_integers();

    // test_empty_input<bg::model::d2::point_xy<int> >();

    return 0;
}
