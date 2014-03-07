// Boost.Geometry Index
// Unit Test

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <geometry_index_test_common.hpp>

#include <boost/geometry/index/detail/algorithms/segment_intersection.hpp>

//#include <boost/geometry/io/wkt/read.hpp>

template <typename Box, typename Point, typename RelativeDistance>
void test_segment_intersection(Box const& box, Point const& p0, Point const& p1,
                               bool expected_result,
                               RelativeDistance expected_rel_dist)
{
    RelativeDistance rel_dist;
    bool value = bgi::detail::segment_intersection(box, p0, p1, rel_dist);
    BOOST_CHECK(value == expected_result);
    if ( value && expected_result )
        BOOST_CHECK_CLOSE(rel_dist, expected_rel_dist, 0.0001);
}

template <typename Box, typename Point, typename RelativeDistance>
void test_geometry(std::string const& wkt_g, std::string const& wkt_p0, std::string const& wkt_p1,
                   bool expected_result,
                   RelativeDistance expected_rel_dist)
{
    Box box;
    bg::read_wkt(wkt_g, box);
    Point p0, p1;
    bg::read_wkt(wkt_p0, p0);
    bg::read_wkt(wkt_p1, p1);
    test_segment_intersection(box, p0, p1, expected_result, expected_rel_dist);
}

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

void test_large_integers()
{
    typedef bg::model::point<int, 2, bg::cs::cartesian> int_point_type;
    typedef bg::model::point<double, 2, bg::cs::cartesian> double_point_type;

    bg::model::box<int_point_type> int_box;
    bg::model::box<double_point_type> double_box;
    int_point_type int_p0, int_p1;
    double_point_type double_p0, double_p1;

    std::string const str_box = "POLYGON((1536119 192000, 1872000 528000))";
    std::string const str_p0 = "POINT(1535000 191000)";
    std::string const str_p1 = "POINT(1873000 529000)";
    bg::read_wkt(str_box, int_box);
    bg::read_wkt(str_box, double_box);
    bg::read_wkt(str_p0, int_p0);
    bg::read_wkt(str_p1, int_p1);
    bg::read_wkt(str_p0, double_p0);
    bg::read_wkt(str_p1, double_p1);

    float int_value;
    bool int_result = bgi::detail::segment_intersection(int_box, int_p0, int_p1, int_value);
    double double_value;
    bool double_result = bgi::detail::segment_intersection(double_box, double_p0, double_p1, double_value);
    BOOST_CHECK(int_result == double_result);
    if ( int_result && double_result )
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
    
    test_geometry<bg::model::box<P2ic>, P2ic>("POLYGON((0 1,2 4))", "POINT(0 0)", "POINT(2 5)", true, 1.0f/5);
    test_geometry<bg::model::box<P2fc>, P2fc>("POLYGON((0 1,2 4))", "POINT(0 0)", "POINT(2 5)", true, 1.0f/5);
    test_geometry<bg::model::box<P2dc>, P2dc>("POLYGON((0 1,2 4))", "POINT(0 0)", "POINT(2 5)", true, 1.0/5);
    test_geometry<bg::model::box<P3ic>, P3ic>("POLYGON((0 1 2,2 4 6))", "POINT(0 0 0)", "POINT(2 5 7)", true, 2.0f/7);
    test_geometry<bg::model::box<P3fc>, P3fc>("POLYGON((0 1 2,2 4 6))", "POINT(0 0 0)", "POINT(2 5 7)", true, 2.0f/7);
    test_geometry<bg::model::box<P3dc>, P3dc>("POLYGON((0 1 2,2 4 6))", "POINT(0 0 0)", "POINT(2 5 7)", true, 2.0/7);

    test_geometry<bg::model::box<P2ic>, P2ic>("POLYGON((0 1,2 4))", "POINT(3 4)", "POINT(0 0)", true, 1.0f/3);
    test_geometry<bg::model::box<P2fc>, P2fc>("POLYGON((0 1,2 4))", "POINT(3 4)", "POINT(0 2)", true, 1.0f/3);
    test_geometry<bg::model::box<P2dc>, P2dc>("POLYGON((0 1,2 4))", "POINT(3 4)", "POINT(0 2)", true, 1.0/3);
    test_geometry<bg::model::box<P3ic>, P3ic>("POLYGON((0 1 2,2 4 6))", "POINT(3 5 6)", "POINT(0 3 3)", true, 1.0f/2);
    test_geometry<bg::model::box<P3fc>, P3fc>("POLYGON((0 1 2,2 4 6))", "POINT(3 5 6)", "POINT(0 3 3)", true, 1.0f/2);
    test_geometry<bg::model::box<P3dc>, P3dc>("POLYGON((0 1 2,2 4 6))", "POINT(3 5 6)", "POINT(0 3 3)", true, 1.0/2);

    test_geometry<bg::model::box<P2ic>, P2ic>("POLYGON((0 1,2 4))", "POINT(1 0)", "POINT(1 5)", true, 1.0f/5);
    test_geometry<bg::model::box<P2fc>, P2fc>("POLYGON((0 1,2 4))", "POINT(1 5)", "POINT(1 0)", true, 1.0f/5);
    test_geometry<bg::model::box<P2dc>, P2dc>("POLYGON((0 1,2 4))", "POINT(1 0)", "POINT(1 5)", true, 1.0/5);
    test_geometry<bg::model::box<P3ic>, P3ic>("POLYGON((0 1 2,2 4 6))", "POINT(1 3 0)", "POINT(1 3 7)", true, 2.0f/7);
    test_geometry<bg::model::box<P3fc>, P3fc>("POLYGON((0 1 2,2 4 6))", "POINT(1 3 7)", "POINT(1 3 0)", true, 1.0f/7);
    test_geometry<bg::model::box<P3dc>, P3dc>("POLYGON((0 1 2,2 4 6))", "POINT(1 3 0)", "POINT(1 3 7)", true, 2.0/7);

    test_geometry<bg::model::box<P2ic>, P2ic>("POLYGON((0 1,2 4))", "POINT(0 0)", "POINT(0 5)", true, 0.2f);
    test_geometry<bg::model::box<P2fc>, P2fc>("POLYGON((0 1,2 4))", "POINT(0 5)", "POINT(0 0)", true, 0.2f);
    test_geometry<bg::model::box<P2dc>, P2dc>("POLYGON((0 1,2 4))", "POINT(0 0)", "POINT(0 5)", true, 0.2);

    test_geometry<bg::model::box<P2ic>, P2ic>("POLYGON((0 1,2 4))", "POINT(3 0)", "POINT(3 5)", false, 0.0f);
    test_geometry<bg::model::box<P2fc>, P2fc>("POLYGON((0 1,2 4))", "POINT(3 5)", "POINT(3 0)", false, 0.0f);
    test_geometry<bg::model::box<P2dc>, P2dc>("POLYGON((0 1,2 4))", "POINT(3 0)", "POINT(3 5)", false, 0.0);

    test_geometry<bg::model::box<P2fc>, P2fc>("POLYGON((0 1,2 4))", "POINT(1 0)", "POINT(1 1)", true, 1.0f);
    test_geometry<bg::model::box<P2fc>, P2fc>("POLYGON((0 1,2 4))", "POINT(1 4)", "POINT(1 5)", true, 0.0f);

    test_geometry<bg::model::box<P2fc>, P2fc>("POLYGON((0 1,2 4))", "POINT(0.5 2)", "POINT(1.5 3)", true, 0.0f);
    
#ifdef HAVE_TTMATH
    typedef bg::model::point<ttmath_big, 2, bg::cs::cartesian> P2ttmc;
    typedef bg::model::point<ttmath_big, 3, bg::cs::cartesian> P3ttmc;

    test_geometry<bg::model::box<P2ttmc>, P2ttmc>("POLYGON((0 1,2 4))", "POINT(0 0)", "POINT(2 5)", true, 1.0f/5);
    test_geometry<bg::model::box<P3ttmc>, P3ttmc>("POLYGON((0 1 2,2 4 6))", "POINT(0 0 0)", "POINT(2 5 7)", true, 2.0f/7);
#endif

    test_large_integers();

    return 0;
}
