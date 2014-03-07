// Boost.Geometry Index
// Unit Test

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>

#include <geometry_index_test_common.hpp>

#include <boost/geometry/index/detail/algorithms/is_valid.hpp>

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

//#define GEOMETRY_TEST_DEBUG

template <typename Geometry>
void test(Geometry const& geometry, bool expected_value)
{
    bool value = bgi::detail::is_valid(geometry);

#ifdef GEOMETRY_TEST_DEBUG
    std::ostringstream out;
    out << typeid(typename bg::coordinate_type<Geometry>::type).name()
        << " "
        << typeid(bool).name()
        << " "
        << "is_valid : " << value
        << std::endl;
    std::cout << out.str();
#endif

    BOOST_CHECK(value == expected_value);
}

template <typename Box>
void test_box(std::string const& wkt, bool expected_value)
{
    Box box;
    bg::read_wkt(wkt, box);
    test(box, expected_value);
    typename bg::point_type<Box>::type temp_pt;
    temp_pt = box.min_corner();
    box.min_corner() = box.max_corner();
    box.max_corner() = temp_pt;
    test(box, !expected_value);
}

void test_large_integers()
{
    typedef bg::model::point<int, 2, bg::cs::cartesian> int_point_type;
    typedef bg::model::point<double, 2, bg::cs::cartesian> double_point_type;

    bg::model::box<int_point_type> int_box;
    bg::model::box<double_point_type> double_box;

    std::string const box_li = "POLYGON((1536119 192000, 1872000 528000))";
    bg::read_wkt(box_li, int_box);
    bg::read_wkt(box_li, double_box);

    BOOST_CHECK(bgi::detail::is_valid(int_box) == bgi::detail::is_valid(double_box));

    std::string const box_li2 = "POLYGON((1872000 528000, 1536119 192000))";
    bg::read_wkt(box_li2, int_box);
    bg::read_wkt(box_li2, double_box);

    BOOST_CHECK(bgi::detail::is_valid(int_box) == bgi::detail::is_valid(double_box));
}

int test_main(int, char* [])
{
    typedef bg::model::point<int, 2, bg::cs::cartesian> P2ic;
    typedef bg::model::point<float, 2, bg::cs::cartesian> P2fc;
    typedef bg::model::point<double, 2, bg::cs::cartesian> P2dc;

    typedef bg::model::point<int, 3, bg::cs::cartesian> P3ic;
    typedef bg::model::point<float, 3, bg::cs::cartesian> P3fc;
    typedef bg::model::point<double, 3, bg::cs::cartesian> P3dc;
    
    test(P2ic(0, 0), true);
    test(P2fc(0, 0), true);
    test(P2dc(0, 0), true);
    test(P3ic(0, 0, 0), true);
    test(P3fc(0, 0, 0), true);
    test(P3dc(0, 0, 0), true);
    
    test_box<bg::model::box<P2ic> >("POLYGON((0 1,2 4))", true);
    test_box<bg::model::box<P2fc> >("POLYGON((0 1,2 4))", true);
    test_box<bg::model::box<P2dc> >("POLYGON((0 1,2 4))", true);
    test_box<bg::model::box<P3ic> >("POLYGON((0 1 2,2 4 6))", true);
    test_box<bg::model::box<P3fc> >("POLYGON((0 1 2,2 4 6))", true);
    test_box<bg::model::box<P3dc> >("POLYGON((0 1 2,2 4 6))", true);
    
#ifdef HAVE_TTMATH
    typedef bg::model::point<ttmath_big, 2, bg::cs::cartesian> P2ttmc;
    typedef bg::model::point<ttmath_big, 3, bg::cs::cartesian> P3ttmc;

    test_geometry<bg::model::box<P2ttmc> >("POLYGON((0 1,2 4))", true);
    test_geometry<bg::model::box<P3ttmc> >("POLYGON((0 1 2,2 4 6))", true);
#endif

    test_large_integers();

    return 0;
}
