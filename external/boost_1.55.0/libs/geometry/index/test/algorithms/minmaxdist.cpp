// Boost.Geometry Index
// Unit Test

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>

#include <geometry_index_test_common.hpp>

#include <boost/geometry/index/detail/algorithms/minmaxdist.hpp>

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

#define GEOMETRY_TEST_DEBUG

template <typename Point, typename Indexable>
void test(Point const& pt, Indexable const& indexable,
    typename bg::default_distance_result<Point, Indexable>::type expected_value)
{
    typename bg::default_distance_result<Point, Indexable>::type value = bgi::detail::minmaxdist(pt, indexable);

#ifdef GEOMETRY_TEST_DEBUG
    std::ostringstream out;
    out << typeid(typename bg::coordinate_type<Point>::type).name()
        << " "
        << typeid(typename bg::coordinate_type<Indexable>::type).name()
        << " "
        << typeid(bg::default_distance_result<Point, Indexable>::type).name()
        << " "
        << "minmaxdist : " << value
        << std::endl;
    std::cout << out.str();
#endif

    BOOST_CHECK_CLOSE(value, expected_value, 0.0001);
}

template <typename Indexable, typename Point>
void test_indexable(Point const& pt, std::string const& wkt,
    typename bg::default_distance_result<Point, Indexable>::type expected_value)
{
    Indexable indexable;
    bg::read_wkt(wkt, indexable);
    test(pt, indexable, expected_value);
}

void test_large_integers()
{
    typedef bg::model::point<int, 2, bg::cs::cartesian> int_point_type;
    typedef bg::model::point<double, 2, bg::cs::cartesian> double_point_type;

    int_point_type int_pt(0, 0);
    double_point_type double_pt(0, 0);

    bg::model::box<int_point_type> int_box;
    bg::model::box<double_point_type> double_box;

    std::string const box_li = "POLYGON((1536119 192000, 1872000 528000))";
    bg::read_wkt(box_li, int_box);
    bg::read_wkt(box_li, double_box);
    
    BOOST_CHECK(bgi::detail::minmaxdist(int_pt, int_box) == bgi::detail::minmaxdist(double_pt, double_box));
}

int test_main(int, char* [])
{
    typedef bg::model::point<int, 2, bg::cs::cartesian> P2ic;
    typedef bg::model::point<float, 2, bg::cs::cartesian> P2fc;
    typedef bg::model::point<double, 2, bg::cs::cartesian> P2dc;

    typedef bg::model::point<int, 3, bg::cs::cartesian> P3ic;
    typedef bg::model::point<float, 3, bg::cs::cartesian> P3fc;
    typedef bg::model::point<double, 3, bg::cs::cartesian> P3dc;

    test_indexable<bg::model::box<P2ic> >(P2ic(1, 2), "POLYGON((0 1,2 4))", 5.0);
    test_indexable<bg::model::box<P2fc> >(P2fc(1, 2), "POLYGON((0 1,2 4))", 5.0);
    test_indexable<bg::model::box<P2dc> >(P2dc(1, 2), "POLYGON((0 1,2 4))", 5.0);
    test_indexable<bg::model::box<P3ic> >(P3ic(1, 2, 3), "POLYGON((0 1 2,2 4 6))", 14.0);
    test_indexable<bg::model::box<P3fc> >(P3fc(1, 2, 3), "POLYGON((0 1 2,2 4 6))", 14.0);
    test_indexable<bg::model::box<P3dc> >(P3dc(1, 2, 3), "POLYGON((0 1 2,2 4 6))", 14.0);

    test_indexable<bg::model::box<P2ic> >(P2ic(1, 2), "POLYGON((1 2,3 5))", 4.0);
    
#ifdef HAVE_TTMATH
    typedef bg::model::point<ttmath_big, 2, bg::cs::cartesian> P2ttmc;
    typedef bg::model::point<ttmath_big, 3, bg::cs::cartesian> P3ttmc;

    test_indexable<bg::model::box<P2ttmc> >(P2ttmc(1, 2), "POLYGON((0 1,2 4))", 5.0);
    test_indexable<bg::model::box<P3ttmc> >(P3ttmc(1, 2, 3), "POLYGON((0 1 2,2 4 6))", 14.0);
#endif

    test_large_integers();

    return 0;
}
